/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 */
#include "drmP.h"
#include "radeon_drm.h"
#include "radeon.h"
#include "radeon_reg.h"

/*
 * GART
 * The GART (Graphics Aperture Remapping Table) is an aperture
 * in the GPU's address space.  System pages can be mapped into
 * the aperture and look like contiguous pages from the GPU's
 * perspective.  A page table maps the pages in the aperture
 * to the actual backing pages in system memory.
 *
 * Radeon GPUs support both an internal GART, as described above,
 * and AGP.  AGP works similarly, but the GART table is configured
 * and maintained by the northbridge rather than the driver.
 * Radeon hw has a separate AGP aperture that is programmed to
 * point to the AGP aperture provided by the northbridge and the
 * requests are passed through to the northbridge aperture.
 * Both AGP and internal GART can be used at the same time, however
 * that is not currently supported by the driver.
 *
 * This file handles the common internal GART management.
 */

/*
 * Common GART table functions.
 */
/**
 * radeon_gart_table_ram_alloc - allocate system ram for gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Allocate system memory for GART page table
 * (r1xx-r3xx, non-pcie r4xx, rs400).  These asics require the
 * gart table to be in system memory.
 * Returns 0 for success, -ENOMEM for failure.
 */
int radeon_gart_table_ram_alloc(struct radeon_device *rdev)
{
	void *ptr;

	ptr = pci_alloc_consistent(rdev->pdev, rdev->gart.table_size,
				   &rdev->gart.table_addr);
	if (ptr == NULL) {
		return -ENOMEM;
	}
#ifdef CONFIG_X86
	if (rdev->family == CHIP_RS400 || rdev->family == CHIP_RS480 ||
	    rdev->family == CHIP_RS690 || rdev->family == CHIP_RS740) {
		set_memory_uc((unsigned long)ptr,
			      rdev->gart.table_size >> PAGE_SHIFT);
	}
#endif
	rdev->gart.ptr = ptr;
	memset((void *)rdev->gart.ptr, 0, rdev->gart.table_size);
	return 0;
}

/**
 * radeon_gart_table_ram_free - free system ram for gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Free system memory for GART page table
 * (r1xx-r3xx, non-pcie r4xx, rs400).  These asics require the
 * gart table to be in system memory.
 */
void radeon_gart_table_ram_free(struct radeon_device *rdev)
{
	if (rdev->gart.ptr == NULL) {
		return;
	}
#ifdef CONFIG_X86
	if (rdev->family == CHIP_RS400 || rdev->family == CHIP_RS480 ||
	    rdev->family == CHIP_RS690 || rdev->family == CHIP_RS740) {
		set_memory_wb((unsigned long)rdev->gart.ptr,
			      rdev->gart.table_size >> PAGE_SHIFT);
	}
#endif
	pci_free_consistent(rdev->pdev, rdev->gart.table_size,
			    (void *)rdev->gart.ptr,
			    rdev->gart.table_addr);
	rdev->gart.ptr = NULL;
	rdev->gart.table_addr = 0;
}

/**
 * radeon_gart_table_vram_alloc - allocate vram for gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Allocate video memory for GART page table
 * (pcie r4xx, r5xx+).  These asics require the
 * gart table to be in video memory.
 * Returns 0 for success, error for failure.
 */
int radeon_gart_table_vram_alloc(struct radeon_device *rdev)
{
	int r;

	if (rdev->gart.robj == NULL) {
		r = radeon_bo_create(rdev, rdev->gart.table_size,
				     PAGE_SIZE, true, RADEON_GEM_DOMAIN_VRAM,
				     NULL, &rdev->gart.robj);
		if (r) {
			return r;
		}
	}
	return 0;
}

/**
 * radeon_gart_table_vram_pin - pin gart page table in vram
 *
 * @rdev: radeon_device pointer
 *
 * Pin the GART page table in vram so it will not be moved
 * by the memory manager (pcie r4xx, r5xx+).  These asics require the
 * gart table to be in video memory.
 * Returns 0 for success, error for failure.
 */
int radeon_gart_table_vram_pin(struct radeon_device *rdev)
{
	uint64_t gpu_addr;
	int r;

	r = radeon_bo_reserve(rdev->gart.robj, false);
	if (unlikely(r != 0))
		return r;
	r = radeon_bo_pin(rdev->gart.robj,
				RADEON_GEM_DOMAIN_VRAM, &gpu_addr);
	if (r) {
		radeon_bo_unreserve(rdev->gart.robj);
		return r;
	}
	r = radeon_bo_kmap(rdev->gart.robj, &rdev->gart.ptr);
	if (r)
		radeon_bo_unpin(rdev->gart.robj);
	radeon_bo_unreserve(rdev->gart.robj);
	rdev->gart.table_addr = gpu_addr;
	return r;
}

/**
 * radeon_gart_table_vram_unpin - unpin gart page table in vram
 *
 * @rdev: radeon_device pointer
 *
 * Unpin the GART page table in vram (pcie r4xx, r5xx+).
 * These asics require the gart table to be in video memory.
 */
void radeon_gart_table_vram_unpin(struct radeon_device *rdev)
{
	int r;

	if (rdev->gart.robj == NULL) {
		return;
	}
	r = radeon_bo_reserve(rdev->gart.robj, false);
	if (likely(r == 0)) {
		radeon_bo_kunmap(rdev->gart.robj);
		radeon_bo_unpin(rdev->gart.robj);
		radeon_bo_unreserve(rdev->gart.robj);
		rdev->gart.ptr = NULL;
	}
}

/**
 * radeon_gart_table_vram_free - free gart page table vram
 *
 * @rdev: radeon_device pointer
 *
 * Free the video memory used for the GART page table
 * (pcie r4xx, r5xx+).  These asics require the gart table to
 * be in video memory.
 */
void radeon_gart_table_vram_free(struct radeon_device *rdev)
{
	if (rdev->gart.robj == NULL) {
		return;
	}
	radeon_gart_table_vram_unpin(rdev);
	radeon_bo_unref(&rdev->gart.robj);
}

/*
 * Common gart functions.
 */
/**
 * radeon_gart_unbind - unbind pages from the gart page table
 *
 * @rdev: radeon_device pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to unbind
 *
 * Unbinds the requested pages from the gart page table and
 * replaces them with the dummy page (all asics).
 */
void radeon_gart_unbind(struct radeon_device *rdev, unsigned offset,
			int pages)
{
	unsigned t;
	unsigned p;
	int i, j;
	u64 page_base;

	if (!rdev->gart.ready) {
		WARN(1, "trying to unbind memory from uninitialized GART !\n");
		return;
	}
	t = offset / RADEON_GPU_PAGE_SIZE;
	p = t / (PAGE_SIZE / RADEON_GPU_PAGE_SIZE);
	for (i = 0; i < pages; i++, p++) {
		if (rdev->gart.pages[p]) {
			rdev->gart.pages[p] = NULL;
			rdev->gart.pages_addr[p] = rdev->dummy_page.addr;
			page_base = rdev->gart.pages_addr[p];
			for (j = 0; j < (PAGE_SIZE / RADEON_GPU_PAGE_SIZE); j++, t++) {
				if (rdev->gart.ptr) {
					radeon_gart_set_page(rdev, t, page_base);
				}
				page_base += RADEON_GPU_PAGE_SIZE;
			}
		}
	}
	mb();
	radeon_gart_tlb_flush(rdev);
}

/**
 * radeon_gart_bind - bind pages into the gart page table
 *
 * @rdev: radeon_device pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to bind
 * @pagelist: pages to bind
 * @dma_addr: DMA addresses of pages
 *
 * Binds the requested pages to the gart page table
 * (all asics).
 * Returns 0 for success, -EINVAL for failure.
 */
int radeon_gart_bind(struct radeon_device *rdev, unsigned offset,
		     int pages, struct page **pagelist, dma_addr_t *dma_addr)
{
	unsigned t;
	unsigned p;
	uint64_t page_base;
	int i, j;

	if (!rdev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}
	t = offset / RADEON_GPU_PAGE_SIZE;
	p = t / (PAGE_SIZE / RADEON_GPU_PAGE_SIZE);

	for (i = 0; i < pages; i++, p++) {
		rdev->gart.pages_addr[p] = dma_addr[i];
		rdev->gart.pages[p] = pagelist[i];
		if (rdev->gart.ptr) {
			page_base = rdev->gart.pages_addr[p];
			for (j = 0; j < (PAGE_SIZE / RADEON_GPU_PAGE_SIZE); j++, t++) {
				radeon_gart_set_page(rdev, t, page_base);
				page_base += RADEON_GPU_PAGE_SIZE;
			}
		}
	}
	mb();
	radeon_gart_tlb_flush(rdev);
	return 0;
}

/**
 * radeon_gart_restore - bind all pages in the gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Binds all pages in the gart page table (all asics).
 * Used to rebuild the gart table on device startup or resume.
 */
void radeon_gart_restore(struct radeon_device *rdev)
{
	int i, j, t;
	u64 page_base;

	if (!rdev->gart.ptr) {
		return;
	}
	for (i = 0, t = 0; i < rdev->gart.num_cpu_pages; i++) {
		page_base = rdev->gart.pages_addr[i];
		for (j = 0; j < (PAGE_SIZE / RADEON_GPU_PAGE_SIZE); j++, t++) {
			radeon_gart_set_page(rdev, t, page_base);
			page_base += RADEON_GPU_PAGE_SIZE;
		}
	}
	mb();
	radeon_gart_tlb_flush(rdev);
}

/**
 * radeon_gart_init - init the driver info for managing the gart
 *
 * @rdev: radeon_device pointer
 *
 * Allocate the dummy page and init the gart driver info (all asics).
 * Returns 0 for success, error for failure.
 */
int radeon_gart_init(struct radeon_device *rdev)
{
	int r, i;

	if (rdev->gart.pages) {
		return 0;
	}
	/* We need PAGE_SIZE >= RADEON_GPU_PAGE_SIZE */
	if (PAGE_SIZE < RADEON_GPU_PAGE_SIZE) {
		DRM_ERROR("Page size is smaller than GPU page size!\n");
		return -EINVAL;
	}
	r = radeon_dummy_page_init(rdev);
	if (r)
		return r;
	/* Compute table size */
	rdev->gart.num_cpu_pages = rdev->mc.gtt_size / PAGE_SIZE;
	rdev->gart.num_gpu_pages = rdev->mc.gtt_size / RADEON_GPU_PAGE_SIZE;
	DRM_INFO("GART: num cpu pages %u, num gpu pages %u\n",
		 rdev->gart.num_cpu_pages, rdev->gart.num_gpu_pages);
	/* Allocate pages table */
	rdev->gart.pages = kzalloc(sizeof(void *) * rdev->gart.num_cpu_pages,
				   GFP_KERNEL);
	if (rdev->gart.pages == NULL) {
		radeon_gart_fini(rdev);
		return -ENOMEM;
	}
	rdev->gart.pages_addr = kzalloc(sizeof(dma_addr_t) *
					rdev->gart.num_cpu_pages, GFP_KERNEL);
	if (rdev->gart.pages_addr == NULL) {
		radeon_gart_fini(rdev);
		return -ENOMEM;
	}
	/* set GART entry to point to the dummy page by default */
	for (i = 0; i < rdev->gart.num_cpu_pages; i++) {
		rdev->gart.pages_addr[i] = rdev->dummy_page.addr;
	}
	return 0;
}

/**
 * radeon_gart_fini - tear down the driver info for managing the gart
 *
 * @rdev: radeon_device pointer
 *
 * Tear down the gart driver info and free the dummy page (all asics).
 */
void radeon_gart_fini(struct radeon_device *rdev)
{
	if (rdev->gart.pages && rdev->gart.pages_addr && rdev->gart.ready) {
		/* unbind pages */
		radeon_gart_unbind(rdev, 0, rdev->gart.num_cpu_pages);
	}
	rdev->gart.ready = false;
	kfree(rdev->gart.pages);
	kfree(rdev->gart.pages_addr);
	rdev->gart.pages = NULL;
	rdev->gart.pages_addr = NULL;

	radeon_dummy_page_fini(rdev);
}

/*
 * GPUVM
 * GPUVM is similar to the legacy gart on older asics, however
 * rather than there being a single global gart table
 * for the entire GPU, there are multiple VM page tables active
 * at any given time.  The VM page tables can contain a mix
 * vram pages and system memory pages and system memory pages
 * can be mapped as snooped (cached system pages) or unsnooped
 * (uncached system pages).
 * Each VM has an ID associated with it and there is a page table
 * associated with each VMID.  When execting a command buffer,
 * the kernel tells the the ring what VMID to use for that command
 * buffer.  VMIDs are allocated dynamically as commands are submitted.
 * The userspace drivers maintain their own address space and the kernel
 * sets up their pages tables accordingly when they submit their
 * command buffers and a VMID is assigned.
 * Cayman/Trinity support up to 8 active VMs at any given time;
 * SI supports 16.
 */

/*
 * vm helpers
 *
 * TODO bind a default page at vm initialization for default address
 */

/**
 * radeon_vm_manager_init - init the vm manager
 *
 * @rdev: radeon_device pointer
 *
 * Init the vm manager (cayman+).
 * Returns 0 for success, error for failure.
 */
int radeon_vm_manager_init(struct radeon_device *rdev)
{
	struct radeon_vm *vm;
	struct radeon_bo_va *bo_va;
	int r;

	if (!rdev->vm_manager.enabled) {
		/* allocate enough for 2 full VM pts */
		r = radeon_sa_bo_manager_init(rdev, &rdev->vm_manager.sa_manager,
					      rdev->vm_manager.max_pfn * 8 * 2,
					      RADEON_GEM_DOMAIN_VRAM);
		if (r) {
			dev_err(rdev->dev, "failed to allocate vm bo (%dKB)\n",
				(rdev->vm_manager.max_pfn * 8) >> 10);
			return r;
		}

		r = radeon_asic_vm_init(rdev);
		if (r)
			return r;

		rdev->vm_manager.enabled = true;

		r = radeon_sa_bo_manager_start(rdev, &rdev->vm_manager.sa_manager);
		if (r)
			return r;
	}

	/* restore page table */
	list_for_each_entry(vm, &rdev->vm_manager.lru_vm, list) {
		if (vm->sa_bo == NULL)
			continue;

		list_for_each_entry(bo_va, &vm->va, vm_list) {
			struct ttm_mem_reg *mem = NULL;
			if (bo_va->valid)
				mem = &bo_va->bo->tbo.mem;

			bo_va->valid = false;
			r = radeon_vm_bo_update_pte(rdev, vm, bo_va->bo, mem);
			if (r) {
				DRM_ERROR("Failed to update pte for vm %d!\n", vm->id);
			}
		}
	}
	return 0;
}

/**
 * radeon_vm_free_pt - free the page table for a specific vm
 *
 * @rdev: radeon_device pointer
 * @vm: vm to unbind
 *
 * Free the page table of a specific vm (cayman+).
 *
 * Global and local mutex must be lock!
 */
static void radeon_vm_free_pt(struct radeon_device *rdev,
				    struct radeon_vm *vm)
{
	struct radeon_bo_va *bo_va;

	if (!vm->sa_bo)
		return;

	list_del_init(&vm->list);
	radeon_sa_bo_free(rdev, &vm->sa_bo, vm->fence);
	vm->pt = NULL;

	list_for_each_entry(bo_va, &vm->va, vm_list) {
		bo_va->valid = false;
	}
}

/**
 * radeon_vm_manager_fini - tear down the vm manager
 *
 * @rdev: radeon_device pointer
 *
 * Tear down the VM manager (cayman+).
 */
void radeon_vm_manager_fini(struct radeon_device *rdev)
{
	struct radeon_vm *vm, *tmp;
	int i;

	if (!rdev->vm_manager.enabled)
		return;

	mutex_lock(&rdev->vm_manager.lock);
	/* free all allocated page tables */
	list_for_each_entry_safe(vm, tmp, &rdev->vm_manager.lru_vm, list) {
		mutex_lock(&vm->mutex);
		radeon_vm_free_pt(rdev, vm);
		mutex_unlock(&vm->mutex);
	}
	for (i = 0; i < RADEON_NUM_VM; ++i) {
		radeon_fence_unref(&rdev->vm_manager.active[i]);
	}
	radeon_asic_vm_fini(rdev);
	mutex_unlock(&rdev->vm_manager.lock);

	radeon_sa_bo_manager_suspend(rdev, &rdev->vm_manager.sa_manager);
	radeon_sa_bo_manager_fini(rdev, &rdev->vm_manager.sa_manager);
	rdev->vm_manager.enabled = false;
}

/**
 * radeon_vm_alloc_pt - allocates a page table for a VM
 *
 * @rdev: radeon_device pointer
 * @vm: vm to bind
 *
 * Allocate a page table for the requested vm (cayman+).
 * Also starts to populate the page table.
 * Returns 0 for success, error for failure.
 *
 * Global and local mutex must be locked!
 */
int radeon_vm_alloc_pt(struct radeon_device *rdev, struct radeon_vm *vm)
{
	struct radeon_vm *vm_evict;
	int r;

	if (vm == NULL) {
		return -EINVAL;
	}

	if (vm->sa_bo != NULL) {
		/* update lru */
		list_del_init(&vm->list);
		list_add_tail(&vm->list, &rdev->vm_manager.lru_vm);
		return 0;
	}

retry:
	r = radeon_sa_bo_new(rdev, &rdev->vm_manager.sa_manager, &vm->sa_bo,
			     RADEON_GPU_PAGE_ALIGN(vm->last_pfn * 8),
			     RADEON_GPU_PAGE_SIZE, false);
	if (r == -ENOMEM) {
		if (list_empty(&rdev->vm_manager.lru_vm)) {
			return r;
		}
		vm_evict = list_first_entry(&rdev->vm_manager.lru_vm, struct radeon_vm, list);
		mutex_lock(&vm_evict->mutex);
		radeon_vm_free_pt(rdev, vm_evict);
		mutex_unlock(&vm_evict->mutex);
		goto retry;

	} else if (r) {
		return r;
	}

	vm->pt = radeon_sa_bo_cpu_addr(vm->sa_bo);
	vm->pt_gpu_addr = radeon_sa_bo_gpu_addr(vm->sa_bo);
	memset(vm->pt, 0, RADEON_GPU_PAGE_ALIGN(vm->last_pfn * 8));

	list_add_tail(&vm->list, &rdev->vm_manager.lru_vm);
	return radeon_vm_bo_update_pte(rdev, vm, rdev->ring_tmp_bo.bo,
				       &rdev->ring_tmp_bo.bo->tbo.mem);
}

/**
 * radeon_vm_grab_id - allocate the next free VMID
 *
 * @rdev: radeon_device pointer
 * @vm: vm to allocate id for
 * @ring: ring we want to submit job to
 *
 * Allocate an id for the vm (cayman+).
 * Returns the fence we need to sync to (if any).
 *
 * Global and local mutex must be locked!
 */
struct radeon_fence *radeon_vm_grab_id(struct radeon_device *rdev,
				       struct radeon_vm *vm, int ring)
{
	struct radeon_fence *best[RADEON_NUM_RINGS] = {};
	unsigned choices[2] = {};
	unsigned i;

	/* check if the id is still valid */
	if (vm->fence && vm->fence == rdev->vm_manager.active[vm->id])
		return NULL;

	/* we definately need to flush */
	radeon_fence_unref(&vm->last_flush);

	/* skip over VMID 0, since it is the system VM */
	for (i = 1; i < rdev->vm_manager.nvm; ++i) {
		struct radeon_fence *fence = rdev->vm_manager.active[i];

		if (fence == NULL) {
			/* found a free one */
			vm->id = i;
			return NULL;
		}

		if (radeon_fence_is_earlier(fence, best[fence->ring])) {
			best[fence->ring] = fence;
			choices[fence->ring == ring ? 0 : 1] = i;
		}
	}

	for (i = 0; i < 2; ++i) {
		if (choices[i]) {
			vm->id = choices[i];
			return rdev->vm_manager.active[choices[i]];
		}
	}

	/* should never happen */
	BUG();
	return NULL;
}

/**
 * radeon_vm_fence - remember fence for vm
 *
 * @rdev: radeon_device pointer
 * @vm: vm we want to fence
 * @fence: fence to remember
 *
 * Fence the vm (cayman+).
 * Set the fence used to protect page table and id.
 *
 * Global and local mutex must be locked!
 */
void radeon_vm_fence(struct radeon_device *rdev,
		     struct radeon_vm *vm,
		     struct radeon_fence *fence)
{
	radeon_fence_unref(&rdev->vm_manager.active[vm->id]);
	rdev->vm_manager.active[vm->id] = radeon_fence_ref(fence);

	radeon_fence_unref(&vm->fence);
	vm->fence = radeon_fence_ref(fence);
}

/* object have to be reserved */
/**
 * radeon_vm_bo_add - add a bo to a specific vm
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @bo: radeon buffer object
 * @offset: requested offset of the buffer in the VM address space
 * @flags: attributes of pages (read/write/valid/etc.)
 *
 * Add @bo into the requested vm (cayman+).
 * Add @bo to the list of bos associated with the vm and validate
 * the offset requested within the vm address space.
 * Returns 0 for success, error for failure.
 */
int radeon_vm_bo_add(struct radeon_device *rdev,
		     struct radeon_vm *vm,
		     struct radeon_bo *bo,
		     uint64_t offset,
		     uint32_t flags)
{
	struct radeon_bo_va *bo_va, *tmp;
	struct list_head *head;
	uint64_t size = radeon_bo_size(bo), last_offset = 0;
	unsigned last_pfn;

	bo_va = kzalloc(sizeof(struct radeon_bo_va), GFP_KERNEL);
	if (bo_va == NULL) {
		return -ENOMEM;
	}
	bo_va->vm = vm;
	bo_va->bo = bo;
	bo_va->soffset = offset;
	bo_va->eoffset = offset + size;
	bo_va->flags = flags;
	bo_va->valid = false;
	INIT_LIST_HEAD(&bo_va->bo_list);
	INIT_LIST_HEAD(&bo_va->vm_list);
	/* make sure object fit at this offset */
	if (bo_va->soffset >= bo_va->eoffset) {
		kfree(bo_va);
		return -EINVAL;
	}

	last_pfn = bo_va->eoffset / RADEON_GPU_PAGE_SIZE;
	if (last_pfn > rdev->vm_manager.max_pfn) {
		kfree(bo_va);
		dev_err(rdev->dev, "va above limit (0x%08X > 0x%08X)\n",
			last_pfn, rdev->vm_manager.max_pfn);
		return -EINVAL;
	}

	mutex_lock(&vm->mutex);
	if (last_pfn > vm->last_pfn) {
		/* release mutex and lock in right order */
		mutex_unlock(&vm->mutex);
		mutex_lock(&rdev->vm_manager.lock);
		mutex_lock(&vm->mutex);
		/* and check again */
		if (last_pfn > vm->last_pfn) {
			/* grow va space 32M by 32M */
			unsigned align = ((32 << 20) >> 12) - 1;
			radeon_vm_free_pt(rdev, vm);
			vm->last_pfn = (last_pfn + align) & ~align;
		}
		mutex_unlock(&rdev->vm_manager.lock);
	}
	head = &vm->va;
	last_offset = 0;
	list_for_each_entry(tmp, &vm->va, vm_list) {
		if (bo_va->soffset >= last_offset && bo_va->eoffset < tmp->soffset) {
			/* bo can be added before this one */
			break;
		}
		if (bo_va->soffset >= tmp->soffset && bo_va->soffset < tmp->eoffset) {
			/* bo and tmp overlap, invalid offset */
			dev_err(rdev->dev, "bo %p va 0x%08X conflict with (bo %p 0x%08X 0x%08X)\n",
				bo, (unsigned)bo_va->soffset, tmp->bo,
				(unsigned)tmp->soffset, (unsigned)tmp->eoffset);
			kfree(bo_va);
			mutex_unlock(&vm->mutex);
			return -EINVAL;
		}
		last_offset = tmp->eoffset;
		head = &tmp->vm_list;
	}
	list_add(&bo_va->vm_list, head);
	list_add_tail(&bo_va->bo_list, &bo->va);
	mutex_unlock(&vm->mutex);
	return 0;
}

/**
 * radeon_vm_get_addr - get the physical address of the page
 *
 * @rdev: radeon_device pointer
 * @mem: ttm mem
 * @pfn: pfn
 *
 * Look up the physical address of the page that the pte resolves
 * to (cayman+).
 * Returns the physical address of the page.
 */
u64 radeon_vm_get_addr(struct radeon_device *rdev,
		       struct ttm_mem_reg *mem,
		       unsigned pfn)
{
	u64 addr = 0;

	switch (mem->mem_type) {
	case TTM_PL_VRAM:
		addr = (mem->start << PAGE_SHIFT);
		addr += pfn * RADEON_GPU_PAGE_SIZE;
		addr += rdev->vm_manager.vram_base_offset;
		break;
	case TTM_PL_TT:
		/* offset inside page table */
		addr = mem->start << PAGE_SHIFT;
		addr += pfn * RADEON_GPU_PAGE_SIZE;
		addr = addr >> PAGE_SHIFT;
		/* page table offset */
		addr = rdev->gart.pages_addr[addr];
		/* in case cpu page size != gpu page size*/
		addr += (pfn * RADEON_GPU_PAGE_SIZE) & (~PAGE_MASK);
		break;
	default:
		break;
	}
	return addr;
}

/* object have to be reserved & global and local mutex must be locked */
/**
 * radeon_vm_bo_update_pte - map a bo into the vm page table
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @bo: radeon buffer object
 * @mem: ttm mem
 *
 * Fill in the page table entries for @bo (cayman+).
 * Returns 0 for success, -EINVAL for failure.
 */
int radeon_vm_bo_update_pte(struct radeon_device *rdev,
			    struct radeon_vm *vm,
			    struct radeon_bo *bo,
			    struct ttm_mem_reg *mem)
{
	struct radeon_bo_va *bo_va;
	unsigned ngpu_pages;
	uint64_t pfn;

	/* nothing to do if vm isn't bound */
	if (vm->sa_bo == NULL)
		return 0;

	bo_va = radeon_bo_va(bo, vm);
	if (bo_va == NULL) {
		dev_err(rdev->dev, "bo %p not in vm %p\n", bo, vm);
		return -EINVAL;
	}

	if (bo_va->valid && mem)
		return 0;

	ngpu_pages = radeon_bo_ngpu_pages(bo);
	bo_va->flags &= ~RADEON_VM_PAGE_VALID;
	bo_va->flags &= ~RADEON_VM_PAGE_SYSTEM;
	if (mem) {
		if (mem->mem_type != TTM_PL_SYSTEM) {
			bo_va->flags |= RADEON_VM_PAGE_VALID;
			bo_va->valid = true;
		}
		if (mem->mem_type == TTM_PL_TT) {
			bo_va->flags |= RADEON_VM_PAGE_SYSTEM;
		}
	}
	if (!bo_va->valid) {
		mem = NULL;
	}
	pfn = bo_va->soffset / RADEON_GPU_PAGE_SIZE;
	radeon_asic_vm_set_page(rdev, bo_va->vm, pfn, mem, ngpu_pages, bo_va->flags);
	radeon_fence_unref(&vm->last_flush);
	return 0;
}

/**
 * radeon_vm_bo_rmv - remove a bo to a specific vm
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @bo: radeon buffer object
 *
 * Remove @bo from the requested vm (cayman+).
 * Remove @bo from the list of bos associated with the vm and
 * remove the ptes for @bo in the page table.
 * Returns 0 for success.
 *
 * Object have to be reserved!
 */
int radeon_vm_bo_rmv(struct radeon_device *rdev,
		     struct radeon_vm *vm,
		     struct radeon_bo *bo)
{
	struct radeon_bo_va *bo_va;

	bo_va = radeon_bo_va(bo, vm);
	if (bo_va == NULL)
		return 0;

	mutex_lock(&rdev->vm_manager.lock);
	mutex_lock(&vm->mutex);
	radeon_vm_free_pt(rdev, vm);
	mutex_unlock(&rdev->vm_manager.lock);
	list_del(&bo_va->vm_list);
	mutex_unlock(&vm->mutex);
	list_del(&bo_va->bo_list);

	kfree(bo_va);
	return 0;
}

/**
 * radeon_vm_bo_invalidate - mark the bo as invalid
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @bo: radeon buffer object
 *
 * Mark @bo as invalid (cayman+).
 */
void radeon_vm_bo_invalidate(struct radeon_device *rdev,
			     struct radeon_bo *bo)
{
	struct radeon_bo_va *bo_va;

	BUG_ON(!atomic_read(&bo->tbo.reserved));
	list_for_each_entry(bo_va, &bo->va, bo_list) {
		bo_va->valid = false;
	}
}

/**
 * radeon_vm_init - initialize a vm instance
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 *
 * Init @vm (cayman+).
 * Map the IB pool and any other shared objects into the VM
 * by default as it's used by all VMs.
 * Returns 0 for success, error for failure.
 */
int radeon_vm_init(struct radeon_device *rdev, struct radeon_vm *vm)
{
	int r;

	vm->id = 0;
	vm->fence = NULL;
	mutex_init(&vm->mutex);
	INIT_LIST_HEAD(&vm->list);
	INIT_LIST_HEAD(&vm->va);
	/* SI requires equal sized PTs for all VMs, so always set
	 * last_pfn to max_pfn.  cayman allows variable sized
	 * pts so we can grow then as needed.  Once we switch
	 * to two level pts we can unify this again.
	 */
	if (rdev->family >= CHIP_TAHITI)
		vm->last_pfn = rdev->vm_manager.max_pfn;
	else
		vm->last_pfn = 0;
	/* map the ib pool buffer at 0 in virtual address space, set
	 * read only
	 */
	r = radeon_vm_bo_add(rdev, vm, rdev->ring_tmp_bo.bo, 0,
			     RADEON_VM_PAGE_READABLE | RADEON_VM_PAGE_SNOOPED);
	return r;
}

/**
 * radeon_vm_fini - tear down a vm instance
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 *
 * Tear down @vm (cayman+).
 * Unbind the VM and remove all bos from the vm bo list
 */
void radeon_vm_fini(struct radeon_device *rdev, struct radeon_vm *vm)
{
	struct radeon_bo_va *bo_va, *tmp;
	int r;

	mutex_lock(&rdev->vm_manager.lock);
	mutex_lock(&vm->mutex);
	radeon_vm_free_pt(rdev, vm);
	mutex_unlock(&rdev->vm_manager.lock);

	/* remove all bo at this point non are busy any more because unbind
	 * waited for the last vm fence to signal
	 */
	r = radeon_bo_reserve(rdev->ring_tmp_bo.bo, false);
	if (!r) {
		bo_va = radeon_bo_va(rdev->ring_tmp_bo.bo, vm);
		list_del_init(&bo_va->bo_list);
		list_del_init(&bo_va->vm_list);
		radeon_bo_unreserve(rdev->ring_tmp_bo.bo);
		kfree(bo_va);
	}
	if (!list_empty(&vm->va)) {
		dev_err(rdev->dev, "still active bo inside vm\n");
	}
	list_for_each_entry_safe(bo_va, tmp, &vm->va, vm_list) {
		list_del_init(&bo_va->vm_list);
		r = radeon_bo_reserve(bo_va->bo, false);
		if (!r) {
			list_del_init(&bo_va->bo_list);
			radeon_bo_unreserve(bo_va->bo);
			kfree(bo_va);
		}
	}
	radeon_fence_unref(&vm->fence);
	radeon_fence_unref(&vm->last_flush);
	mutex_unlock(&vm->mutex);
}
