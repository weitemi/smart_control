/*
 * @Author: your name
 * @Date: 2021-09-02 10:13:47
 * @LastEditTime: 2021-09-02 14:59:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \smart_control\main\app\etymology.c
 */
#include "etymology.h"

static int do_object(char *p)
{
	int obj = -1;
	if (strstr(p, "天气"))
	{
		obj = 1;
	}
	if (strstr(p, "空调"))
	{
		obj = 2;
	}
	if (strstr(p, "蓝牙"))
	{
		obj = 3;
	}
	return obj;
}
static int do_action(char *p)
{
	int action = -1;
	if (strstr(p, "打开") || strstr(p, "设置"))
	{
		action = 1;
	}
	if (strstr(p, "关闭"))
	{
		action = 0;
	}
	return action;
}

static time_tt do_time(char *p)
{
	time_tt t;

	if (strstr(p, "明天"))
	{
		t.day = 1;
	}
	if (strstr(p, "后天"))
	{
		t.day = 2;
	}
	if (strstr(p, "今天"))
	{
		t.day = 0;
	}
	if (strstr(p, "早上"))
	{
		t.sig = 0;
	}
	if (strstr(p, "下午") || strstr(p, "晚上"))
	{
		t.sig = 1;
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
static int do_number(char *string)
{
	//?获取文本中的数字
	char *p = strstr(string, "度");
	if (p)
	{
		char num_str[5] = {0};
		char *w = num_str;
		while (*p != '\0')
		{
			if (*p > 47 && *p < 58 && w < w + 5)
			{
				*w = *p;
				w++;
			}
			p++;
		}
		if (num_str[0] != 0)
			return atoi(num_str);
	}

	return -1;
}
order_t etymology(char *s)
{
	order_t ord;
	ord.obj = do_object(s);
	ord.action = do_action(s);
	ord.time = do_time(s);
	ord.number = do_number(s);
	return ord;
}
