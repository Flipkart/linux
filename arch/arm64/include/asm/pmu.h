/*
 * Based on arch/arm/include/asm/pmu.h
 *
 * Copyright (C) 2009 picoChip Designs Ltd, Jamie Iles
 * Copyright (C) 2012 ARM Ltd.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_PMU_H
#define __ASM_PMU_H

/*
 * Per-CPU PMCR: config reg
 */
#define ARMV8_PMCR_E		(1 << 0) /* Enable all counters */
#define ARMV8_PMCR_P		(1 << 1) /* Reset all counters */
#define ARMV8_PMCR_C		(1 << 2) /* Cycle counter reset */
#define ARMV8_PMCR_D		(1 << 3) /* CCNT counts every 64th cpu cycle */
#define ARMV8_PMCR_X		(1 << 4) /* Export to ETM */
#define ARMV8_PMCR_DP		(1 << 5) /* Disable CCNT if non-invasive debug*/
#define	ARMV8_PMCR_N_SHIFT	11	 /* Number of counters supported */
#define	ARMV8_PMCR_N_MASK	0x1f
#define	ARMV8_PMCR_MASK		0x3f	 /* Mask for writable bits */

/*
 * PMCNTEN: counters enable reg
 */
#define	ARMV8_CNTEN_MASK	0xffffffff	/* Mask for writable bits */

/*
 * PMINTEN: counters interrupt enable reg
 */
#define	ARMV8_INTEN_MASK	0xffffffff	/* Mask for writable bits */

/*
 * PMOVSR: counters overflow flag status reg
 */
#define	ARMV8_OVSR_MASK		0xffffffff	/* Mask for writable bits */
#define	ARMV8_OVERFLOWED_MASK	ARMV8_OVSR_MASK

/*
 * PMXEVTYPER: Event selection reg
 */
#define	ARMV8_EVTYPE_MASK	0xc80003ff	/* Mask for writable bits */
#define	ARMV8_EVTYPE_EVENT	0x3ff		/* Mask for EVENT bits */

/*
 * Event filters for PMUv3
 */
#define	ARMV8_EXCLUDE_EL1	(1 << 31)
#define	ARMV8_EXCLUDE_EL0	(1 << 30)
#define	ARMV8_INCLUDE_EL2	(1 << 27)

#ifndef __ASSEMBLY__
#ifdef CONFIG_HW_PERF_EVENTS

/* The events for a given PMU register set. */
struct pmu_hw_events {
	/*
	 * The events that are active on the PMU for the given index.
	 */
	struct perf_event	**events;

	/*
	 * A 1 bit for an index indicates that the counter is being used for
	 * an event. A 0 means that the counter can be used.
	 */
	unsigned long           *used_mask;

	/*
	 * Hardware lock to serialize accesses to PMU registers. Needed for the
	 * read/modify/write sequences.
	 */
	raw_spinlock_t		pmu_lock;
};

struct arm_pmu {
	struct pmu		pmu;
	cpumask_t		active_irqs;
	const char		*name;
	irqreturn_t		(*handle_irq)(int irq_num, void *dev);
	void			(*enable)(struct hw_perf_event *evt, int idx);
	void			(*disable)(struct hw_perf_event *evt, int idx);
	int			(*get_event_idx)(struct pmu_hw_events *hw_events,
						 struct hw_perf_event *hwc);
	int			(*set_event_filter)(struct hw_perf_event *evt,
						    struct perf_event_attr *attr);
	u32			(*read_counter)(int idx);
	void			(*write_counter)(int idx, u32 val);
	void			(*start)(void);
	void			(*stop)(void);
	void			(*reset)(void *);
	int			(*map_event)(struct perf_event *event);
	int			num_events;
	atomic_t		active_events;
	struct mutex		reserve_mutex;
	u64			max_period;
	struct platform_device	*plat_device;
	struct pmu_hw_events	*(*get_hw_events)(void);
};

#define to_arm_pmu(p) (container_of(p, struct arm_pmu, pmu))

int __init armpmu_register(struct arm_pmu *armpmu, char *name, int type);

u64 armpmu_event_update(struct perf_event *event,
			struct hw_perf_event *hwc,
			int idx);

int armpmu_event_set_period(struct perf_event *event,
			    struct hw_perf_event *hwc,
			    int idx);

#endif /* CONFIG_HW_PERF_EVENTS */
#endif /* __ASSEMBLY__ */
#endif /* __ASM_PMU_H */
