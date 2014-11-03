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

#include <linux/cpu.h>
#include <linux/kvm.h>
#include <linux/kvm_host.h>

#include <kvm/arm_vgic.h>
#include <kvm/arm_pmu.h>

/**
 * kvm_pmu_sync_hwstate - sync pmu state for cpu
 * @vcpu: The vcpu pointer
 *
 * Inject virtual PMU IRQ if IRQ is pending for this cpu.
 */
void kvm_pmu_sync_hwstate(struct kvm_vcpu *vcpu)
{
	struct pmu_cpu *pmu = &vcpu->arch.pmu_cpu;
	struct pmu_kvm *kpmu = &vcpu->kvm->arch.pmu;

	if (pmu->irq_pending) {
		kvm_vgic_inject_irq(vcpu->kvm, vcpu->vcpu_id,
				    kpmu->irq_num[vcpu->vcpu_id],
				    1);
		pmu->irq_pending = 0;
		return;
	}
}

/**
 * kvm_pmu_vcpu_reset - reset pmu state for cpu
 * @vcpu: The vcpu pointer
 *
 */
void kvm_pmu_vcpu_reset(struct kvm_vcpu *vcpu)
{
	struct pmu_cpu *pmu = &vcpu->arch.pmu_cpu;

	pmu->irq_pending = 0;
}

/**
 * kvm_pmu_addr - set or get PMU VM IRQ numbers
 * @kvm:   pointer to the vm struct
 * @cpu:  cpu number
 * @irq:  pointer to irq number value
 * @write: if true set the irq number else read the irq number
 *
 * Set or get the PMU IRQ number for the given cpu number.
 */
int kvm_pmu_addr(struct kvm *kvm, unsigned long cpu, u64 *irq, bool write)
{
	struct pmu_kvm *kpmu = &kvm->arch.pmu;

	if (CONFIG_KVM_ARM_MAX_VCPUS <= cpu)
		return -ENODEV;

	mutex_lock(&kvm->lock);

	if (write) {
		kpmu->irq_num[cpu] = *irq;
	} else {
		*irq = kpmu->irq_num[cpu];
	}

	mutex_unlock(&kvm->lock);

	return 0;
}

/**
 * kvm_pmu_init - Initialize global PMU state for a VM
 * @kvm: pointer to the kvm struct
 *
 * Set all the PMU IRQ numbers to invalid value so that
 * user space has to explicitly provide PMU IRQ numbers
 * using set device address ioctl.
 */
int kvm_pmu_init(struct kvm *kvm)
{
	int i;
	struct pmu_kvm *kpmu = &kvm->arch.pmu;

	for (i = 0; i < CONFIG_KVM_ARM_MAX_VCPUS; i++) {
		kpmu->irq_num[i] = UINT_MAX;
	}

	return 0;
}
