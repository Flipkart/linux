/*
 * Copyright (C) 2014 Linaro Ltd.
 * Author: Anup Patel <anup.patel@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __ASM_ARM_KVM_PMU_H
#define __ASM_ARM_KVM_PMU_H

struct pmu_kvm {
#ifdef CONFIG_KVM_ARM_PMU
	/* PMU IRQ Numbers */
	unsigned int		irq_num[CONFIG_KVM_ARM_MAX_VCPUS];
#endif
};

struct pmu_cpu {
#ifdef CONFIG_KVM_ARM_PMU
	/* IRQ pending flag. Updated when registers are saved. */
	u32			irq_pending;
#endif
};

#ifdef CONFIG_KVM_ARM_PMU
void kvm_pmu_vcpu_reset(struct kvm_vcpu *vcpu);
void kvm_pmu_sync_hwstate(struct kvm_vcpu *vcpu);
int kvm_pmu_addr(struct kvm *kvm, unsigned long cpu, u64 *irq, bool write);
int kvm_pmu_init(struct kvm *kvm);
#else
static inline void kvm_pmu_vcpu_reset(struct kvm_vcpu *vcpu) {}
static inline void kvm_pmu_sync_hwstate(struct kvm_vcpu *vcpu) {}
static inline int kvm_pmu_addr(struct kvm *kvm,
				unsigned long cpu, u64 *irq, bool write)
{
	return -ENXIO;
}
static inline int kvm_pmu_init(struct kvm *kvm) { return 0; }
#endif

#endif
