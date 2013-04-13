/*
** Copyright (c) 2009 ARM Ltd. All rights reserved.
*/
#include    "../../armlinux/config.h"
#ifdef L2CACHE_ENABLE
#include    "pl310_l2cc.h"

typedef volatile struct  tagPL310_REG
{
  // Register r0
  const volatile unsigned CacheID;              // 0x000
  const volatile unsigned CacheType;            // 0x004
  volatile unsigned Reserved0[(0x100-0x08)/4];  // 0x008-0x0FC

  // Register r1
  volatile unsigned Ctrl;                       // 0x100
  volatile unsigned AuxCtrl;                    // 0x104
  volatile unsigned TagRAMLatencyCtrl;          // 0x108
  volatile unsigned DataRAMLatencyCtrl;         // 0x10C
  volatile unsigned Reserved1[(0x200-0x110)/4]; // 0x110-0x1FC

  // Register r2
  volatile unsigned EvtCtrCtrl;                 // 0x200
  volatile unsigned EvtCtr1Cnfg;                // 0x204
  volatile unsigned EvtCtr0Cnfg;                // 0x208
  volatile unsigned EvtCtr1Val;                 // 0x20C
  volatile unsigned EvtCtr0Val;                 // 0x210
  volatile unsigned IntrMask;                   // 0x214
  const volatile unsigned MaskIntrStatus;       // 0x218
  const volatile unsigned RawIntrStatus;        // 0x21C
  volatile unsigned IntrClr;                    // 0x220 (WO)
  volatile unsigned Reserved2[(0x730-0x224)/4]; // 0x224-0x72c

  // Register r7
  volatile unsigned CacheSync;                   // 0x730
  volatile unsigned Reserved71[(0x770-0x734)/4]; // 0x734-0x76C
  volatile unsigned InvalLineByPA;               // 0x770
  volatile unsigned Reserved72[(0x77C-0x774)/4]; // 0x774-0x778
  volatile unsigned InvalByWay;                  // 0x77C
  volatile unsigned Reserved73[(0x7B0-0x780)/4]; // 0x780-0x7AC
  volatile unsigned CleanLineByPA;               // 0x7B0
  volatile unsigned Reserved74;                  // 0x7B4
  volatile unsigned CleanLineByIndexWay;         // 0x7B8
  volatile unsigned CleanByWay;                  // 0x7BC
  volatile unsigned Reserved75[(0x7F0-0x7C0)/4]; // 0x7C0-0x7EC
  volatile unsigned CleanInvalByPA;              // 0x7F0
  volatile unsigned Reserved76;                  // 0x7F4
  volatile unsigned CleanInvalByIndexWay;        // 0x7F8
  volatile unsigned CleanInvalByWay;             // 0x7FC
  volatile unsigned Reserved77[(0x900-0x800)/4]; // 0x800-0x8FC

  // Register r9
  volatile unsigned DataLockdown0ByWay;          // 0x900
  volatile unsigned InstrLockdown0ByWay;         // 0x904
  volatile unsigned DataLockdown1ByWay;          // 0x908
  volatile unsigned InstrLockdown1ByWay;         // 0x90C
  volatile unsigned DataLockdown2ByWay;          // 0x910
  volatile unsigned InstrLockdown2ByWay;         // 0x914
  volatile unsigned DataLockdown3ByWay;          // 0x918
  volatile unsigned InstrLockdown3ByWay;         // 0x91C
  volatile unsigned DataLockdown4ByWay;          // 0x920
  volatile unsigned InstrLockdown4ByWay;         // 0x924
  volatile unsigned DataLockdown5ByWay;          // 0x928
  volatile unsigned InstrLockdown5ByWay;         // 0x92C
  volatile unsigned DataLockdown6ByWay;          // 0x930
  volatile unsigned InstrLockdown6ByWay;         // 0x934
  volatile unsigned DataLockdown7ByWay;          // 0x938
  volatile unsigned InstrLockdown7ByWay;         // 0x93C
  volatile unsigned Reserved90[(0x950-0x940)/4]; // 0x940-0x94C
  volatile unsigned LockdownByLineEnable;        // 0x950
  volatile unsigned UnlockAllLinesByWay;         // 0x954
  volatile unsigned Reserved91[(0xC00-0x958)/4]; // 0x958-0x9FC

  // Register r12
  volatile unsigned AddressFilteringStart;       // 0xC00
  volatile unsigned AddressFilteringEnd;         // 0xC04
  volatile unsigned Reserved12[(0xF40-0xC08)/4]; // 0xC08-0xF3C

  // Register r15
  volatile unsigned DebugCtrl;                   // 0xF40
  volatile unsigned Reserved13[(0xF60-0xF44)/4]; // 0xF44-0xF7C
  volatile unsigned PrefetchCtrl;                // 0xF60
  volatile unsigned Reserved14[(0xF80-0xF64)/4]; // 0xF64-0xF7C
  volatile unsigned PowerCtrl;                   // 0xF80
} PL310_REG, *pPL310_REG;;

#define g_pl310reg ((pPL310_REG)L2C_BASE)
#define L2_LY_SP_OFF (0)
#define L2_LY_SP_MSK (0x7)

#define L2_LY_RD_OFF (4)
#define L2_LY_RD_MSK (0x7)

#define L2_LY_WR_OFF (8)
#define L2_LY_WR_MSK (0x7)
#define L2_LY_SET(ly,off) (((ly)-1)<<(off))

#define CACHE_LINE_SIZE		32

uint32 c_L2CACHE_WAY_SIZE = 2;
uint32 c_L2CACHE_WAY_NUM = 1;

void L2CacheLineConfig(uint32 Waysize,uint32 WayNum)
{
    c_L2CACHE_WAY_SIZE = Waysize;
    c_L2CACHE_WAY_NUM = WayNum;
}

static uint32 l2x0_way_mask;	/* Bitmask of active ways */
static uint32 l2x0_size;
static uint32 l2x0_cache_id;
static uint32 l2x0_sets;
static uint32 l2x0_ways;

static void cache_sync(void)
{
    g_pl310reg->CacheSync = 0;
}


static void l2x0_inv_all(void)
{
	g_pl310reg->InvalByWay = l2x0_way_mask;
	while (g_pl310reg->InvalByWay & l2x0_way_mask);
	cache_sync();
}

static void __l2x0_flush_all(void)
{
	g_pl310reg->CleanInvalByWay = l2x0_way_mask;
	while (g_pl310reg->CleanInvalByWay & l2x0_way_mask);
	cache_sync();
}

static void l2x0_flush_line(unsigned long addr)
{
	while (g_pl310reg->CleanInvalByPA & 1);
	g_pl310reg->CleanInvalByPA = addr;
}

void l2x0_flush_range(unsigned long start, unsigned long end)
{
	unsigned long flags;

	if ((end - start) >= l2x0_size) {
		__l2x0_flush_all();
		return;
	}
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + MIN(end - start, 4096UL);
		while (start < blk_end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
		}
	}
	while (g_pl310reg->CleanInvalByPA & 1);
	//cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	cache_sync();
}

void L2x0Init(void)
{
	uint32 aux;
	uint32 way_size = 0;
	uint32 aux_val, aux_mask;
	
    g_pl310reg->TagRAMLatencyCtrl = L2_LY_SET(1,L2_LY_SP_OFF)|L2_LY_SET(1,L2_LY_RD_OFF)|L2_LY_SET(1,L2_LY_WR_OFF);
    g_pl310reg->DataRAMLatencyCtrl = L2_LY_SET(2,L2_LY_SP_OFF)|L2_LY_SET(4,L2_LY_RD_OFF)|L2_LY_SET(1,L2_LY_WR_OFF);
	//L2X0 Power Control
    g_pl310reg->PowerCtrl = L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
    //16-way associativity, parity disabled ; Way size - 32KB
	aux_val =((c_L2CACHE_WAY_NUM << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) | // 16-way
			(0x1 << 25) | 	// round-robin
			(0x1 << 0) |		// Full Line of Zero Enable
			(0x1 << L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT) |
			(c_L2CACHE_WAY_SIZE << L2X0_AUX_CTRL_WAY_SIZE_SHIFT) | // 32KB way-size
			(0x1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) |
			(0x1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT) |
			(0x1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT) );

	aux_mask = ~((1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) | // 16-way
			(0x1 << 25) | 	// round-robin
			(0x1 << 0) |		// Full Line of Zero Enable
			(0x1 << L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT) |
			(0x7 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT) | // 32KB way-size
			(0x1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) |
			(0x1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT) |
			(0x1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT) );
	
	l2x0_cache_id = g_pl310reg->CacheID;
	aux = g_pl310reg->AuxCtrl;

	aux &= aux_mask;
	aux |= aux_val;

	// Determine the number of ways
	switch (l2x0_cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		if (aux & (1 << 16))
			l2x0_ways = 16;
		else
			l2x0_ways = 8;
		break;
	case L2X0_CACHE_ID_PART_L210:
		l2x0_ways = (aux >> 13) & 0xf;
		break;
	default:
		// Assume unknown chips have 8 ways
		l2x0_ways = 8;
		break;
	}

	l2x0_way_mask = (1 << l2x0_ways) - 1;

	//L2 cache Size =  Way size * Number of ways
	way_size = (aux & L2X0_AUX_CTRL_WAY_SIZE_MASK) >> 17;
	//way_size = SZ_1K << (way_size + 3);
	way_size = 1024 << (way_size + 3);
	l2x0_size = l2x0_ways * way_size;
	l2x0_sets = way_size / CACHE_LINE_SIZE;

	 // Check if l2x0 controller is already enabled.
	 // If you are booting from non-secure mode
	 // accessing the below registers will fault.
	if (!(g_pl310reg->Ctrl & 1)) {

		// l2x0 controller is disabled 
        g_pl310reg->AuxCtrl = aux;
		l2x0_inv_all();

		// enable L2X0
		g_pl310reg->Ctrl = 1;
	}

    g_pl310reg->PrefetchCtrl = 0x7800000E;
	//printf("l2x0: %d ways, CACHE_ID 0x%08x, AUX_CTRL 0x%08x, Cache size: %d B\n",
	//		l2x0_ways, l2x0_cache_id, aux, l2x0_size);
}


void L2x0Deinit(void)
{
	unsigned long flags;
	if(g_pl310reg->Ctrl)
	{
	    __l2x0_flush_all();
	}
	g_pl310reg->Ctrl = 0;
}

#endif

