#include "include/package.h"
#include "include/mempool.h"
#include "thread.h"
static uint8_t pkg_buf[(sizeof(list_node_t) + sizeof(pkg_t)) * PKG_LIMIT];
static uint8_t pkg_databuf[(sizeof(list_node_t) + sizeof(pkg_dblk_t)) * PKG_DATA_BLK_LIMIT];

static lock_t pkg_locker;
static mempool_t pkg_pool;
static mempool_t pkg_datapool;

net_err_t package_pool_init(void)
{
    net_err_t ret;
    printf("package pool init ing ...\n");

    ret = lock_init(&pkg_locker);
    CHECK_NET_ERR(ret);
    ret = mempool_init(&pkg_pool, pkg_buf, PKG_LIMIT, sizeof(pkg_t));
    CHECK_NET_ERR(ret);
    ret = mempool_init(&pkg_datapool, pkg_databuf, PKG_DATA_BLK_LIMIT, sizeof(pkg_dblk_t));
    CHECK_NET_ERR(ret);
    printf("package pool init finsh!...\n");
    return NET_ERR_OK;
}
net_err_t package_pool_destory(void)
{
    net_err_t ret;
    printf("package pool destory...\n");
    ret = lock_destory(&pkg_locker);
    CHECK_NET_ERR(ret);
    mempool_destroy(&pkg_pool);
    mempool_destroy(&pkg_datapool);
    printf("package pool destory finish! ...\n");
    return NET_ERR_OK;
}

static void package_init(pkg_t *pkg, int size)
{
    pkg->total = size;
    list_init(&pkg->pkgdb_list);
    pkg->node.next = NULL;
    pkg->node.pre = NULL;
}
void package_destory(pkg_t *package)
{
    package->total = 0;
    package->node.next = NULL;
    package->node.pre = NULL;
    list_destory(&package->pkgdb_list);
}
#include <string.h>
void package_datablk_destory(pkg_dblk_t *blk)
{
    blk->node.next = NULL;
    blk->node.pre = NULL;
    blk->offset = 0;
    blk->size = 0;
    memset(blk->data,0,PKG_DATA_BLK_SIZE);
}

net_err_t package_collect(pkg_t *package)
{
    net_err_t ret;
    CHECK_NET_NULL(package);

    // 先回收数据块
    while (list_count(&package->pkgdb_list) > 0)
    {

        list_node_t *dnode = list_remove_first(&package->pkgdb_list);
        pkg_dblk_t *blk = list_node_parent(dnode, pkg_dblk_t, node);
        package_datablk_destory(blk);
        ret = mempool_free_blk(&pkg_datapool, blk);
        CHECK_NET_ERR(ret);
    }
    package_destory(package);
    return NET_ERR_OK;
}

pkg_t *package_create(int size)
{
    net_err_t ret;
    pkg_t *package = mempool_alloc_blk(&pkg_pool, -1);
    if (package == NULL)
    {
        return NULL;
    }
    package_init(package, size);

    while (size > 0)
    {
        pkg_dblk_t *dblk = mempool_alloc_blk(&pkg_datapool, -1);
        if (dblk == NULL)
        {
            ret = package_collect(package);
            if (ret != NET_ERR_OK)
                dbg_error("not ok here\n");
            return NULL;
        }
        dblk->node.next = NULL;
        dblk->node.pre = NULL;
        dblk->size = (size >= PKG_DATA_BLK_SIZE) ? PKG_DATA_BLK_SIZE : size;
        dblk->offset = 0;
        list_insert_last(&package->pkgdb_list, &dblk->node);
        size -= dblk->size;
    }
    return package;
}
