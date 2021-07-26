
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// pmm.cpp for Simple-XX/SimpleKernel.

#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "common.h"
#include "pmm.h"

const void *PMM::start                   = nullptr;
size_t      PMM::length                  = 0;
size_t      PMM::total_pages             = 0;
const void *PMM::kernel_space_start      = nullptr;
size_t      PMM::kernel_space_length     = 0;
const void *PMM::non_kernel_space_start  = nullptr;
size_t      PMM::non_kernel_space_length = 0;
ALLOCATOR * PMM::allocator               = nullptr;
ALLOCATOR * PMM::kernel_space_allocator  = nullptr;

PMM::PMM(void) {
    return;
}

PMM::~PMM(void) {
    return;
}

bool PMM::init(void) {
    // 针对不同平台获取物理内存信息，设置内存使用方式
    helper();
    // 创建分配器
    // 内核空间
    static FIRSTFIT first_fit_allocator_kernel(
        "First Fit Allocator(kernel space)", kernel_space_start,
        kernel_space_length / COMMON::PAGE_SIZE);
    kernel_space_allocator = (ALLOCATOR *)&first_fit_allocator_kernel;

    // 非内核空间
    static FIRSTFIT first_fit_allocator(
        "First Fit Allocator", non_kernel_space_start,
        non_kernel_space_length / COMMON::PAGE_SIZE);
    allocator = (ALLOCATOR *)&first_fit_allocator;

    // 内核实际占用页数 这里也算了 0～1M 的 reserved 内存
    size_t kernel_pages =
        ((uint8_t *)COMMON::ALIGN(COMMON::KERNEL_END_ADDR, COMMON::PAGE_SIZE) -
         (uint8_t *)COMMON::ALIGN(COMMON::KERNEL_START_ADDR,
                                  COMMON::PAGE_SIZE)) /
        COMMON::PAGE_SIZE;
    // 将内核已使用部分划分出来
    if (alloc_pages_kernel(const_cast<void *>(COMMON::KERNEL_START_ADDR),
                           kernel_pages) == true) {
        // 将 multiboot2 / dtb 信息移动到内核空间
        move_boot_info();
        printf("pmm init.\n");
        return true;
    }
    else {
        return false;
    }
}

size_t PMM::get_pmm_length(void) {
    return length;
}

void *PMM::alloc_page(void) {
    return allocator->alloc(1);
}

void *PMM::alloc_pages(size_t _len) {
    return allocator->alloc(_len);
}

bool PMM::alloc_pages(void *_addr, size_t _len) {
    return allocator->alloc(_addr, _len);
}

void *PMM::alloc_page_kernel(void) {
    return kernel_space_allocator->alloc(1);
}

void *PMM::alloc_pages_kernel(size_t _len) {
    return kernel_space_allocator->alloc(_len);
}

bool PMM::alloc_pages_kernel(void *_addr, size_t _len) {
    return kernel_space_allocator->alloc(_addr, _len);
}

void PMM::free_page(void *_addr) {
    // 判断应该使用哪个分配器
    if (_addr >= kernel_space_start &&
        _addr < (uint8_t *)kernel_space_start + kernel_space_length) {
        kernel_space_allocator->free(_addr, 1);
    }
    else if (_addr >= non_kernel_space_start &&
             _addr <
                 (uint8_t *)non_kernel_space_start + non_kernel_space_length) {
        allocator->free(_addr, 1);
    }
    else {
        // 如果都不是说明有问题
        assert(0);
    }
    return;
}

void PMM::free_pages(void *_addr, size_t _len) {
    // 判断应该使用哪个分配器
    if (_addr >= kernel_space_start &&
        _addr < (uint8_t *)kernel_space_start + kernel_space_length) {
        kernel_space_allocator->free(_addr, _len);
    }
    else if (_addr >= non_kernel_space_start &&
             _addr <
                 (uint8_t *)non_kernel_space_start + non_kernel_space_length) {
        allocator->free(_addr, _len);
    }
    // 如果都不是说明有问题
    else {
        assert(0);
    }
    return;
}

uint64_t PMM::get_used_pages_count(void) {
    return kernel_space_allocator->get_used_count() +
           allocator->get_used_count();
}

uint64_t PMM::get_free_pages_count(void) {
    return kernel_space_allocator->get_free_count() +
           allocator->get_free_count();
}
