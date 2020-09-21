#include <ctype.h>
#include "ya_breaker_env.h"
#include "ef_types.h"

#define NV_PWR_ON_SWITCH_CTR_MODE "pwrOnRelayCtr"
#define NV_SWITCH_STATUS "RelayStat"
#define INCHING_TIME_VAL "inchT"
#define INCHING_ENABLE_VAL "inchEnble"

long atol(const char *nptr)
{
  int c; /* current char */
  long total; /* current total */
  int sign; /* if '-', then negative, otherwise positive */

  /* skip whitespace */
  while ( isspace((int)(unsigned char)*nptr) )
  ++nptr;

  c = (int)(unsigned char)*nptr++; sign = c; /* save sign indication */
  if (c == '-' || c == '+')
  c = (int)(unsigned char)*nptr++; /* skip sign */

  total = 0;

  while (isdigit(c)) {
  total = 10 * total + (c - '0'); /* accumulate digit */
  c = (int)(unsigned char)*nptr++; /* get next char */
  }

  if (sign == '-')
  return -total;
  else
  return total; /* return result, negated if necessary */
}

int ya_save_pwrOn_switch_ctrMode(pwr_on_relay_status_t ctrType)
{
	if (0 != ef_set_char(NV_PWR_ON_SWITCH_CTR_MODE, (char)ctrType))
	{
		return -1;
	}
	
	return 0;
}

pwr_on_relay_status_t ya_get_pwrOn_switch_ctrMode(void)
{
	return ef_get_char(NV_PWR_ON_SWITCH_CTR_MODE);
}

int ya_save_switch_status(int val)
{
	if (0 != ef_set_char(NV_SWITCH_STATUS, (char)val))
	{
		return -1;
	}
	
	return 0;
}

int ya_get_switch_status(void)
{
	return ef_get_char(NV_SWITCH_STATUS);
}

int ya_get_inching_time(void)
{
	int val;

	val = ef_get_int(INCHING_TIME_VAL);
	if (0 == val)
	{
		val = 1;
	}

	return val;
}

int ya_save_inching_time(int val)
{
	if (0 != ef_set_int(INCHING_TIME_VAL, val))
	{
		return -1;
	}

	return 0;
}

int ya_get_inching_enable_val(void)
{
	return ef_get_char(INCHING_ENABLE_VAL);
}

int ya_save_inching_enable_val(int val)
{
	if (0 != ef_set_char(INCHING_ENABLE_VAL, (char)val))
	{
		return -1;
	}
	
	return 0;
}

