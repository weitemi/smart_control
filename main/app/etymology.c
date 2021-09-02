/*
 * @Author: your name
 * @Date: 2021-09-02 10:13:47
 * @LastEditTime: 2021-09-02 20:14:57
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \smart_control\main\app\etymology.c
 */
#include "etymology.h"

static int do_object(char *p)
{
	int obj = 0;
	if (strstr(p, "天气"))
	{
		obj = OBJ_WEATHER;
	}
	if (strstr(p, "空调"))
	{
		obj = OBJ_AIR_CONDITION;
	}
	if (strstr(p, "蓝牙"))
	{
		obj = OBJ_BLE;
	}
	return obj;
}
static int do_action(char *p)
{
	int action = 0;
	if (strstr(p, "打开") || strstr(p, "设置"))
	{
		action = ACTION_ON;
	}
	if (strstr(p, "关闭"))
	{
		action = ACTION_OFF;
	}
	return action;
}

static time_tt do_time(char *p)
{
	time_tt t={0,0,0,0};

	if (strstr(p, "今天"))
	{
		t.day = TIME_TODAY;
	}
	if (strstr(p, "明天"))
	{
		t.day = TIME_TOMORROW;
	}
	if (strstr(p, "后天"))
	{
		t.day = TIME_ADFTERMORROW;
	}
	if (strstr(p, "早上"))
	{
		t.sig = SIG_AM;
	}
	if (strstr(p, "下午") || strstr(p, "晚上"))
	{
		t.sig = SIG_PM;
	}
	if (strstr(p, "点"))
	{
		char numstr[3] = {0};
		char *w = numstr;
		char *r = strstr(p, "点") - 2;

		if (*r > 47 && *r < 58)
		{
			*w = *r;
			w++;
		}

		r++;
		*w = *r;
		t.hour = atoi(numstr);
		printf("%d点", t.hour);
	}
	if (strstr(p, "分"))
	{
		char numstr[3] = {0};
		char *w = numstr;
		char *r = strstr(p, "分") - 2;

		if (*r > 47 && *r < 58)
		{
			*w = *r;
			w++;
		}

		r++;
		*w = *r;
		t.min = atoi(numstr);
		printf("%d分", t.min);
	}
	return t;
}
//读取温度数值
static int do_number(char *p)
{
	int res = 0;
	if (strstr(p, "度"))
	{
		char numstr[3] = {0};
		char *w = numstr;
		char *r = strstr(p, "度") - 2;

		if (*r > 47 && *r < 58)
		{
			*w = *r;
			w++;
		}

		r++;
		*w = *r;
		res = atoi(numstr);
		printf("%d度", res);
	}

	return res;
}
order_t etymology(char *s)
{
	order_t ord ;
	ord.obj = do_object(s);
	ord.action = do_action(s);
	ord.time = do_time(s);
	ord.number = do_number(s);
	return ord;
}
