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
    lock(&pkg_locker);
    pkg->total = size;
    list_init(&pkg->pkgdb_list);
    pkg->node.next = NULL;
    pkg->node.pre = NULL;
    unlock(&pkg_locker);
}
static void package_destory(pkg_t *package)
{
    lock(&pkg_locker);
    package->total = 0;
    package->node.next = NULL;
    package->node.pre = NULL;
    list_destory(&package->pkgdb_list);
    unlock(&pkg_locker);
}
#include <string.h>
static void package_datablk_destory(pkg_dblk_t *blk)
{
    lock(&pkg_locker);
    blk->node.next = NULL;
    blk->node.pre = NULL;
    blk->offset = 0;
    blk->size = 0;
    memset(blk->data, 0, PKG_DATA_BLK_SIZE);
    unlock(&pkg_locker);
}

net_err_t package_collect(pkg_t *package)
{
    net_err_t ret;
    CHECK_NET_NULL(package);
    lock(&pkg_locker);
    // 先回收数据块
    while (list_count(&package->pkgdb_list) > 0)
    {

        list_node_t *dnode = list_remove_first(&package->pkgdb_list);
        pkg_dblk_t *blk = list_node_parent(dnode, pkg_dblk_t, node);
        package_datablk_destory(blk);
        ret = mempool_free_blk(&pkg_datapool, blk);
        if (ret != NET_ERR_OK)
        {
            unlock(&pkg_locker);
            return ret;
        }
    }
    package_destory(package);
    ret = mempool_free_blk(&pkg_pool, package);
    unlock(&pkg_locker);
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
    lock(&pkg_locker);
    package_init(package, size);

    while (size > 0)
    {
        pkg_dblk_t *dblk = mempool_alloc_blk(&pkg_datapool, -1);
        if (dblk == NULL)
        {
            ret = package_collect(package);
            if (ret != NET_ERR_OK)
                dbg_error("not ok here\n");
            unlock(&pkg_locker);
            return NULL;
        }
        dblk->node.next = NULL;
        dblk->node.pre = NULL;
        dblk->size = (size >= PKG_DATA_BLK_SIZE) ? PKG_DATA_BLK_SIZE : size;
        dblk->offset = 0;
        list_insert_last(&package->pkgdb_list, &dblk->node);
        size -= dblk->size;
    }
    unlock(&pkg_locker);
    return package;
}

net_err_t package_expand_front(pkg_t *package, int ex_size)
{
    lock(&pkg_locker);
    package->total += ex_size;
    list_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *fdata = list_node_parent(fnode, pkg_dblk_t, node);
    int front_leave = fdata->offset;
    if (front_leave >= ex_size)
    {
        fdata->offset -= ex_size;
        fdata->size += ex_size;
    }
    else
    {
        int oringal_offset = fdata->offset;
        int oringal_size = fdata->size;
        fdata->offset = 0;
        fdata->size = PKG_DATA_BLK_SIZE;
        int new_size = ex_size - front_leave;
        pkg_dblk_t *new_blk = mempool_alloc_blk(&pkg_datapool, -1);
        if (new_blk == NULL)
        {
            dbg_warning("pkg data blk alloc failed\n");
            package->total -= ex_size;
            fdata->offset = oringal_offset;
            fdata->size = oringal_size;
            unlock(&pkg_locker);
            return NET_ERR_MEM;
        }
        new_blk->size = new_size;
        new_blk->offset = PKG_DATA_BLK_SIZE - new_size;
        list_insert_first(&package->pkgdb_list, &new_blk->node);
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

net_err_t package_expand_last(pkg_t *package, int ex_size)
{
    lock(&pkg_locker);
    package->total += ex_size;
    list_t *lnode = list_last(&package->pkgdb_list);
    pkg_dblk_t *lblk = list_node_parent(lnode, pkg_dblk_t, node);
    int last_leave = PKG_DATA_BLK_SIZE - (lblk->offset + lblk->size);
    if (last_leave >= ex_size)
    {
        lblk->size += ex_size;
    }
    else
    {
        int original_offset = lblk->offset;
        int original_size = lblk->size;
        lblk->size = PKG_DATA_BLK_SIZE;
        lblk->offset = 0;
        pkg_dblk_t *blk = mempool_alloc_blk(&pkg_datapool, -1);
        if (blk == NULL)
        {
            dbg_warning("pkg data blk alloc failed\n");
            package->total -= ex_size;
            lblk->size = original_size;
            lblk->offset = original_offset;
            unlock(&pkg_locker);
            return NET_ERR_MEM;
        }
        int new_size = ex_size - last_leave;
        blk->size = new_size;
        blk->offset = 0;
        list_insert_last(&package->pkgdb_list, &blk->node);
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

net_err_t package_expand_front_align(pkg_t *package, int ex_size)
{
    lock(&pkg_locker);
    package->total += ex_size;
    list_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *fdata = list_node_parent(fnode, pkg_dblk_t, node);
    int front_leave = fdata->offset;
    if (front_leave >= ex_size)
    {
        fdata->offset -= ex_size;
        fdata->size += ex_size;
    }
    else
    {
        pkg_dblk_t *new_blk = mempool_alloc_blk(&pkg_datapool, -1);
        if (new_blk == NULL)
        {
            package->total -= ex_size;
            unlock(&pkg_locker);
            return NET_ERR_MEM;
        }
        new_blk->offset = PKG_DATA_BLK_SIZE - ex_size;
        new_blk->size = ex_size;
        list_insert_first(&package->pkgdb_list, &new_blk->node);
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

/*show*/
net_err_t package_show_pool_info(void)
{
    printf("package pool info.....................\n");
    printf("package pool free pkg cnt is %d\n", mempool_freeblk_cnt(&pkg_pool));
    printf("pkg data pool free blk cnt is %d\n", mempool_freeblk_cnt(&pkg_datapool));
    printf("\n");
    return NET_ERR_OK;
}
net_err_t package_show_info(pkg_t *package)
{
    if (package == NULL)
    {
        return NET_ERR_SYS;
    }
    int count = 0, i = 1;
    lock(&pkg_locker);
    list_node_t *cur = &package->node;
    while (cur)
    {
        count++;
        cur = cur->next;
    }
    printf("package info ......................\n");
    printf("pkg total size is: %d\n", package->total);
    printf("pkg brother link list count(include itself): %d\n", count);
    list_node_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *curblk = list_node_parent(fnode, pkg_dblk_t, node);
    while (curblk)
    {
        printf("blk[%d].size=%d, .offset=%d\n",
               i++, curblk->size, curblk->offset);
        curblk = package_get_next_datablk(curblk);
    }
    printf("pkg sum data block cnt is %d\n", i - 1);
    unlock(&pkg_locker);
    printf("\n");
    return NET_ERR_OK;
}
/**
 * 给数据包添加包头
 */
net_err_t package_add_header(pkg_t *package, int h_size)
{
    return package_expand_front_align(package, h_size);
}
pkg_dblk_t *package_get_next_datablk(pkg_dblk_t *cur_blk)
{
    lock(&pkg_locker);
    list_node_t *next_node = cur_blk->node.next;
    if (next_node == NULL)
    {
        unlock(&pkg_locker);
        return NULL;
    }
    pkg_dblk_t *next_blk = list_node_parent(next_node, pkg_dblk_t, node);
    unlock(&pkg_locker);
    return next_blk;
}
net_err_t package_remove_one_blk(pkg_t *package, pkg_dblk_t *delblk)
{
    if (package == NULL || delblk == NULL)
    {
        return NET_ERR_SYS;
    }
    list_t *list = &package->pkgdb_list;
    list_node_t *delnode = &delblk->node;
    list_remove(list, delnode);
    return NET_ERR_OK;
}
/**
 * 将所有数据包头整合到一个数据块中
 */
net_err_t package_integrate_header(pkg_t *package, int all_hsize)
{
    if (all_hsize > PKG_DATA_BLK_SIZE)
    {
        dbg_error("header size greater than data block size!\n");
        return NET_ERR_SYS;
    }
    lock(&pkg_locker);
    list_node_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *fblk = list_node_parent(fnode, pkg_dblk_t, node);
    if (fblk->size >= all_hsize)
    {
        unlock(&pkg_locker);
        return NET_ERR_OK;
    }
    else
    {
        int i, j;
        uint8_t *dest = fblk->data;
        uint8_t *src = fblk->data + fblk->offset;
        for (i = 0; i < fblk->size; i++)
        {
            dest[i] = src[i];
        }
        pkg_dblk_t *next = package_get_next_datablk(fblk);
        if (next == NULL)
        {
            unlock(&pkg_locker);
            dbg_error("header size may be not correct\n");
            return NET_ERR_SYS;
        }
        src = next->data + next->offset;
        int leave = all_hsize - i;
        next->size -= leave;
        next->offset += leave;

        for (j = 0; j < leave; j++, i++)
        {
            dest[i] = src[j];
        }

        if (next->size == 0)
        {
            package_remove_one_blk(package, next);
        }
        fblk->offset = 0;
        fblk->size += leave;
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}
