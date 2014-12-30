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
#include <linux/of_irq.h>
#include <linux/kvm.h>
#include <linux/kvm_host.h>
#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/interrupt.h>
#include <asm/kvm_emulate.h>

#include <kvm/arm_vgic.h>
#include <kvm/arm_pmu.h>

static DEFINE_PER_CPU(int, host_pmu_irq);
static DEFINE_PER_CPU(int, host_pmu_irq_disabled);

/**
 * kvm_pmu_switch_host2guest - switch PMU context from host to guest
 * @vcpu: The vcpu pointer
 */
void kvm_pmu_switch_host2guest(struct kvm_vcpu *vcpu)
{
	int hpmu_irq = per_cpu(host_pmu_irq, vcpu->cpu);
	struct pmu_kvm *kpmu = &vcpu->kvm->arch.pmu;

	/* Skip if PMU irq not available for current host CPU */
	if (hpmu_irq < 0)
		return;

	/* Disable PMU irq to avoid infinite VM exits due to host PMU irq */
	per_cpu(host_pmu_irq_disabled, vcpu->cpu) =
			irqd_irq_disabled(irq_get_irq_data(hpmu_irq));
	if (!per_cpu(host_pmu_irq_disabled, vcpu->cpu)) {
		disable_irq(hpmu_irq);
	}

	/* Inject virtual irq if host PMU irq active */
	if (kvm_pmu_overflowed(vcpu)) {
		kvm_vgic_inject_irq(vcpu->kvm, vcpu->vcpu_id,
				    kpmu->irq_num[vcpu->vcpu_id],
				    1);
	}

	/* Switch PMU context from host to guest */
	__kvm_pmu_switch_host2guest(vcpu);
}

/**
 * kvm_pmu_switch_guest2host - switch PMU context from guest to host
 * @vcpu: The vcpu pointer
 */
void kvm_pmu_switch_guest2host(struct kvm_vcpu *vcpu)
{
	int hpmu_irq = per_cpu(host_pmu_irq, vcpu->cpu);

	/* Skip if PMU irq not available for current host CPU */
	if (hpmu_irq < 0)
		return;

	/* Switch PMU context from guest to host */
	__kvm_pmu_switch_guest2host(vcpu);

	/* Re-enable PMU irq if it was enabled previously */
	if (!per_cpu(host_pmu_irq_disabled, vcpu->cpu)) {
		enable_irq(hpmu_irq);
	}
}

/**
 * kvm_pmu_addr - set or get PMU VM IRQ numbers
 * @kvm: pointer to the vm struct
 * @cpu: cpu number
 * @irq: pointer to irq number value
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

static const struct of_device_id pmu_of_match[] = {
	{ .compatible	= "arm,cortex-a15-pmu",	},
	{ .compatible	= "arm,armv8-pmuv3",	},
	{},
};

int kvm_pmu_hyp_init(void)
{
	int cpu, irq;
	struct device_node *np;

	/* Ensure that PMU irq for each possible cpu is invalid */
	for_each_possible_cpu(cpu) {
		per_cpu(host_pmu_irq, cpu) = -1;
		per_cpu(host_pmu_irq_disabled, cpu) = 1;
	}

	/* Find PMU device node */
	np = of_find_matching_node(NULL, pmu_of_match);
	if (!np) {
		kvm_err("kvm_pmu: can't find DT node\n");
		return -ENODEV;
	}

	/* Atleast one irq number should be available */
	irq = of_irq_get(np, 0);
	if (irq <= 0) {
		kvm_err("kvm_pmu: invalid irq\n");
		return -ENODEV;
	}

	/* Find out PMU irq for each possible cpu */
	if (irq_is_percpu(irq)) {
		for_each_possible_cpu(cpu)
			per_cpu(host_pmu_irq, cpu) = irq;
	} else {
		for_each_possible_cpu(cpu) {
			irq = of_irq_get(np, cpu);
			if (irq <= 0) {
				kvm_err("kvm_pmu: invalid irq for CPU%d\n", cpu);
				return -ENODEV;
			}
			per_cpu(host_pmu_irq, cpu) = irq;
		}
	}

	return 0;
}
