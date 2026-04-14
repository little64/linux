// SPDX-License-Identifier: GPL-2.0-only

#include <linux/blk-mq.h>
#include <linux/bio.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

enum little64_pvblk_register {
	L64_PVBLK_MAGIC			= 0x00,
	L64_PVBLK_VERSION		= 0x08,
	L64_PVBLK_SECTOR_SIZE		= 0x10,
	L64_PVBLK_SECTOR_COUNT		= 0x18,
	L64_PVBLK_MAX_SECTORS		= 0x20,
	L64_PVBLK_FEATURES		= 0x28,
	L64_PVBLK_STATUS		= 0x30,
	L64_PVBLK_REQUEST_ADDR		= 0x38,
	L64_PVBLK_KICK			= 0x40,
	L64_PVBLK_IRQ_ACK		= 0x48,
};

enum little64_pvblk_feature_bits {
	L64_PVBLK_F_READ_ONLY		= BIT(0),
	L64_PVBLK_F_FLUSH		= BIT(1),
};

enum little64_pvblk_status_bits {
	L64_PVBLK_S_READY		= BIT(0),
	L64_PVBLK_S_BUSY		= BIT(1),
	L64_PVBLK_S_ERROR		= BIT(2),
	L64_PVBLK_S_IRQ_PENDING		= BIT(3),
};

enum little64_pvblk_req_op {
	L64_PVBLK_REQ_READ		= 0,
	L64_PVBLK_REQ_WRITE		= 1,
	L64_PVBLK_REQ_FLUSH		= 2,
};

enum little64_pvblk_req_status {
	L64_PVBLK_REQ_ST_OK		= 0,
	L64_PVBLK_REQ_ST_IOERR		= 1,
	L64_PVBLK_REQ_ST_RANGE		= 2,
	L64_PVBLK_REQ_ST_UNSUPPORTED	= 3,
	L64_PVBLK_REQ_ST_READ_ONLY	= 4,
	L64_PVBLK_REQ_ST_INVALID	= 5,
};

#define L64_PVBLK_MAGIC_VALUE	0x4B4C42505634364cULL
#define L64_PVBLK_VERSION_VALUE	1ULL

struct little64_pvblk_request {
	u64 op;
	u64 status;
	u64 sector;
	u64 sector_count;
	u64 buffer_phys;
	u64 buffer_len;
	u64 reserved0;
	u64 reserved1;
};

struct little64_pvblk {
	struct device *dev;
	void __iomem *base;
	int irq;

	u64 sector_size;
	u64 sector_count;
	u64 max_sectors;
	u64 features;

	spinlock_t lock;
	struct request *active_req;

	struct blk_mq_tag_set tag_set;
	struct gendisk *disk;

	struct little64_pvblk_request *request_desc;
	void *bounce;
	unsigned int bounce_bytes;
};

static inline u64 l64_pvblk_readq(struct little64_pvblk *pvblk, u32 reg)
{
	return readq(pvblk->base + reg);
}

static inline void l64_pvblk_writeq(struct little64_pvblk *pvblk, u32 reg, u64 value)
{
	writeq(value, pvblk->base + reg);
}

static blk_status_t l64_pvblk_req_status_to_blk(u64 status)
{
	switch (status) {
	case L64_PVBLK_REQ_ST_OK:
		return BLK_STS_OK;
	case L64_PVBLK_REQ_ST_READ_ONLY:
		return BLK_STS_TARGET;
	case L64_PVBLK_REQ_ST_RANGE:
	case L64_PVBLK_REQ_ST_INVALID:
	case L64_PVBLK_REQ_ST_UNSUPPORTED:
	case L64_PVBLK_REQ_ST_IOERR:
	default:
		return BLK_STS_IOERR;
	}
}

static void l64_pvblk_copy_from_request(struct request *rq, void *buffer)
{
	struct req_iterator iter;
	struct bio_vec bvec;
	unsigned int copied = 0;

	rq_for_each_segment(bvec, rq, iter) {
		void *page_addr = kmap_local_page(bvec.bv_page);
		memcpy(buffer + copied, page_addr + bvec.bv_offset, bvec.bv_len);
		kunmap_local(page_addr);
		copied += bvec.bv_len;
	}
}

static void l64_pvblk_copy_to_request(struct request *rq, const void *buffer)
{
	struct req_iterator iter;
	struct bio_vec bvec;
	unsigned int copied = 0;

	rq_for_each_segment(bvec, rq, iter) {
		void *page_addr = kmap_local_page(bvec.bv_page);
		memcpy(page_addr + bvec.bv_offset, buffer + copied, bvec.bv_len);
		kunmap_local(page_addr);
		copied += bvec.bv_len;
	}
}

static irqreturn_t l64_pvblk_irq(int irq, void *dev_id)
{
	struct little64_pvblk *pvblk = dev_id;
	struct request *rq;
	unsigned long flags;
	u64 status = l64_pvblk_readq(pvblk, L64_PVBLK_STATUS);
	blk_status_t blk_sts;

	if (!(status & L64_PVBLK_S_IRQ_PENDING))
		return IRQ_NONE;

	l64_pvblk_writeq(pvblk, L64_PVBLK_IRQ_ACK, 1);

	spin_lock_irqsave(&pvblk->lock, flags);
	rq = pvblk->active_req;
	pvblk->active_req = NULL;
	spin_unlock_irqrestore(&pvblk->lock, flags);

	if (!rq)
		return IRQ_HANDLED;

	if (req_op(rq) == REQ_OP_READ)
		l64_pvblk_copy_to_request(rq, pvblk->bounce);

	blk_sts = l64_pvblk_req_status_to_blk(READ_ONCE(pvblk->request_desc->status));
	blk_mq_end_request(rq, blk_sts);
	return IRQ_HANDLED;
}

static blk_status_t l64_pvblk_queue_rq(struct blk_mq_hw_ctx *hctx,
					 const struct blk_mq_queue_data *bd)
{
	struct request *rq = bd->rq;
	struct little64_pvblk *pvblk = hctx->queue->queuedata;
	unsigned long flags;
	u64 request_bytes = blk_rq_bytes(rq);
	u64 request_sectors = request_bytes >> SECTOR_SHIFT;
	u64 op;

	if (blk_rq_is_passthrough(rq))
		return BLK_STS_IOERR;

	switch (req_op(rq)) {
	case REQ_OP_READ:
		op = L64_PVBLK_REQ_READ;
		break;
	case REQ_OP_WRITE:
		op = L64_PVBLK_REQ_WRITE;
		break;
	case REQ_OP_FLUSH:
		op = L64_PVBLK_REQ_FLUSH;
		request_bytes = 0;
		request_sectors = 0;
		break;
	default:
		return BLK_STS_NOTSUPP;
	}

	if ((request_bytes & (pvblk->sector_size - 1)) != 0)
		return BLK_STS_IOERR;
	if (request_sectors > pvblk->max_sectors)
		return BLK_STS_RESOURCE;
	if (blk_rq_pos(rq) > pvblk->sector_count ||
	    request_sectors > pvblk->sector_count - blk_rq_pos(rq))
		return BLK_STS_IOERR;

	spin_lock_irqsave(&pvblk->lock, flags);
	if (pvblk->active_req) {
		spin_unlock_irqrestore(&pvblk->lock, flags);
		return BLK_STS_RESOURCE;
	}
	pvblk->active_req = rq;
	spin_unlock_irqrestore(&pvblk->lock, flags);

	blk_mq_start_request(rq);

	if (op == L64_PVBLK_REQ_WRITE)
		l64_pvblk_copy_from_request(rq, pvblk->bounce);

	pvblk->request_desc->op = op;
	pvblk->request_desc->status = L64_PVBLK_REQ_ST_INVALID;
	pvblk->request_desc->sector = blk_rq_pos(rq);
	pvblk->request_desc->sector_count = request_sectors;
	pvblk->request_desc->buffer_phys = virt_to_phys(pvblk->bounce);
	pvblk->request_desc->buffer_len = request_bytes;
	pvblk->request_desc->reserved0 = 0;
	pvblk->request_desc->reserved1 = 0;

	wmb();
	l64_pvblk_writeq(pvblk, L64_PVBLK_REQUEST_ADDR, virt_to_phys(pvblk->request_desc));
	wmb();
	l64_pvblk_writeq(pvblk, L64_PVBLK_KICK, 1);
	return BLK_STS_OK;
}

static const struct blk_mq_ops l64_pvblk_mq_ops = {
	.queue_rq = l64_pvblk_queue_rq,
};

static const struct block_device_operations l64_pvblk_fops = {
	.owner = THIS_MODULE,
};

static int l64_pvblk_probe(struct platform_device *pdev)
{
	struct little64_pvblk *pvblk;
	struct queue_limits lim = { };
	u64 magic;
	int ret;

	pvblk = devm_kzalloc(&pdev->dev, sizeof(*pvblk), GFP_KERNEL);
	if (!pvblk)
		return -ENOMEM;

	pvblk->dev = &pdev->dev;
	pvblk->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(pvblk->base))
		return PTR_ERR(pvblk->base);

	pvblk->irq = platform_get_irq(pdev, 0);
	if (pvblk->irq < 0)
		return pvblk->irq;

	magic = l64_pvblk_readq(pvblk, L64_PVBLK_MAGIC);
	if (magic != L64_PVBLK_MAGIC_VALUE)
		return dev_err_probe(&pdev->dev, -ENODEV,
					   "unexpected magic 0x%llx\n", magic);
	if (l64_pvblk_readq(pvblk, L64_PVBLK_VERSION) != L64_PVBLK_VERSION_VALUE)
		return dev_err_probe(&pdev->dev, -ENODEV,
					   "unsupported device version\n");

	pvblk->sector_size = l64_pvblk_readq(pvblk, L64_PVBLK_SECTOR_SIZE);
	pvblk->sector_count = l64_pvblk_readq(pvblk, L64_PVBLK_SECTOR_COUNT);
	pvblk->max_sectors = l64_pvblk_readq(pvblk, L64_PVBLK_MAX_SECTORS);
	pvblk->features = l64_pvblk_readq(pvblk, L64_PVBLK_FEATURES);
	if (pvblk->sector_size != SECTOR_SIZE || !pvblk->sector_count || !pvblk->max_sectors)
		return dev_err_probe(&pdev->dev, -EINVAL,
					   "invalid geometry sectors=%llu sector_size=%llu max=%llu\n",
					   pvblk->sector_count, pvblk->sector_size, pvblk->max_sectors);

	pvblk->bounce_bytes = pvblk->max_sectors * pvblk->sector_size;
	pvblk->bounce = kmalloc(pvblk->bounce_bytes, GFP_KERNEL);
	if (!pvblk->bounce)
		return -ENOMEM;

	pvblk->request_desc = kzalloc(sizeof(*pvblk->request_desc), GFP_KERNEL);
	if (!pvblk->request_desc) {
		ret = -ENOMEM;
		goto err_free_bounce;
	}

	spin_lock_init(&pvblk->lock);

	ret = devm_request_irq(&pdev->dev, pvblk->irq, l64_pvblk_irq, 0,
			      dev_name(&pdev->dev), pvblk);
	if (ret)
		goto err_free_desc;

	lim.logical_block_size = pvblk->sector_size;
	lim.physical_block_size = pvblk->sector_size;
	lim.max_hw_sectors = pvblk->max_sectors;

	pvblk->tag_set.ops = &l64_pvblk_mq_ops;
	pvblk->tag_set.nr_hw_queues = 1;
	pvblk->tag_set.queue_depth = 1;
	pvblk->tag_set.numa_node = NUMA_NO_NODE;
	pvblk->tag_set.cmd_size = 0;
	pvblk->tag_set.driver_data = pvblk;

	ret = blk_mq_alloc_tag_set(&pvblk->tag_set);
	if (ret)
		goto err_free_desc;

	pvblk->disk = blk_mq_alloc_disk(&pvblk->tag_set, &lim, pvblk);
	if (IS_ERR(pvblk->disk)) {
		ret = PTR_ERR(pvblk->disk);
		goto err_free_tag_set;
	}

	pvblk->disk->flags |= GENHD_FL_NO_PART;
	pvblk->disk->fops = &l64_pvblk_fops;
	pvblk->disk->private_data = pvblk;
	pvblk->disk->queue->queuedata = pvblk;
	strscpy(pvblk->disk->disk_name, "l64blk0", DISK_NAME_LEN);
	if (pvblk->features & L64_PVBLK_F_READ_ONLY)
		set_disk_ro(pvblk->disk, true);
	set_capacity(pvblk->disk, pvblk->sector_count);

	ret = device_add_disk(&pdev->dev, pvblk->disk, NULL);
	if (ret)
		goto err_put_disk;

	platform_set_drvdata(pdev, pvblk);
	dev_info(&pdev->dev, "Little64 PV block disk: %llu sectors (%s)\n",
		 pvblk->sector_count,
		 (pvblk->features & L64_PVBLK_F_READ_ONLY) ? "ro" : "rw");
	return 0;

err_put_disk:
	put_disk(pvblk->disk);
err_free_tag_set:
	blk_mq_free_tag_set(&pvblk->tag_set);
err_free_desc:
	kfree(pvblk->request_desc);
err_free_bounce:
	kfree(pvblk->bounce);
	return ret;
}

static void l64_pvblk_remove(struct platform_device *pdev)
{
	struct little64_pvblk *pvblk = platform_get_drvdata(pdev);

	if (!pvblk)
		return;

	del_gendisk(pvblk->disk);
	put_disk(pvblk->disk);
	blk_mq_free_tag_set(&pvblk->tag_set);
	kfree(pvblk->request_desc);
	kfree(pvblk->bounce);
}

static const struct of_device_id l64_pvblk_of_match[] = {
	{ .compatible = "little64,pvblk" },
	{ }
};

static struct platform_driver l64_pvblk_driver = {
	.probe = l64_pvblk_probe,
	.remove = l64_pvblk_remove,
	.driver = {
		.name = "little64-pvblk",
		.of_match_table = l64_pvblk_of_match,
	},
};
builtin_platform_driver(l64_pvblk_driver);