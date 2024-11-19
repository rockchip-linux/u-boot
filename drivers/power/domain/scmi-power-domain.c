// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Fuzhou Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <power-domain.h>
#include <power-domain-uclass.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>

static int scmi_power_domain_state_set(struct power_domain *power_domain,
				       u32 enable)
{
	struct scmi_pwd_state_set_in in = {
		.domain_id = power_domain->id,
		.pstate = enable,
	};
	struct scmi_pwd_state_set_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_POWER_DOMAIN,
					  SCMI_POWER_DOMAIN_STATE_SET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(power_domain->dev->parent, &msg);
	if (ret)
		return ret;

	return scmi_to_linux_errno(out.status);
}

static int scmi_power_domain_on(struct power_domain *power_domain)
{
	return scmi_power_domain_state_set(power_domain, 0);
}

static int scmi_power_domain_off(struct power_domain *power_domain)
{
	return scmi_power_domain_state_set(power_domain, SCMI_PWD_PSTATE_TYPE_LOST);
}

static struct power_domain_ops scmi_power_domain_ops = {
	.on = scmi_power_domain_on,
	.off = scmi_power_domain_off,
};

U_BOOT_DRIVER(scmi_power_domain) = {
	.name = "scmi_power_domain",
	.id = UCLASS_POWER_DOMAIN,
	.ops = &scmi_power_domain_ops,
};
