/*
 * @Author: your name
 * @Date: 2020-12-19 16:48:59
 * @LastEditTime: 2021-09-02 23:57:45
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adfd:\MyNote\clk_t\clk_t.c
 */
#include "clock.h"
#include "mywifi.h"
#include "myhttp.h"



static const char *TAG = "clock.c";

const int UPDATE_CLK = BIT0;
const int NEW_TIMER = BIT1;

EventGroupHandle_t clk_event;

struct timer *timer_list = NULL;
clk_t global_clk; //全局时间

/*
 * 保存时间结构体，使其成员符合时间规则
 * clk：要更新的时间
 * 返回：0 年份更新；1 年份未更新
 */
int save_clk(clk_t *clk)
{

	if (clk->cal.second >= 60)
	{
		//one minute
		clk->cal.second = clk->cal.second - 60;
		clk->cal.minute++;
		if (clk->cal.minute >= 60)
		{
			//one hour
			clk->cal.minute = clk->cal.minute - 60;
			clk->cal.hour++;
			if (clk->cal.hour >= 24)
			{
				//date +1
				clk->cal.hour = clk->cal.hour - 24;
				clk->cal.date = clk->cal.date + 1;
				switch (clk->cal.month)
				{
				case 1:
				case 3:
				case 5:
				case 7:
				case 8:
				case 10:
				case 12:
					if (clk->cal.date == 32)
					{

						goto exit;
					}
					break;
				case 2:
					//29 or 28 in february
					if (((clk->cal.year % 4) == 0 && clk->cal.date == 30) || ((clk->cal.year % 4 != 0) && (clk->cal.date == 29)))
					{

						goto exit;
					}
					break;
				default:
					if (clk->cal.date == 31)
					{
						goto exit;
					}
					break;
				}
			}
		}
	}
	return 1;
//month +1
exit:
	clk->cal.date = 1;
	clk->cal.month++;
	if (clk->cal.month == 13)
	{
		clk->cal.month = 1;
		clk->cal.year++; //max to 2064
	}
	return 0;
}

/*
 * 将定时器插入链表，按时间先后顺序
 * tmr：要插入的定时器
 * 返回：0失败；1成功
 */
int tmr_add(struct timer *tmr)
{
	struct timer *t, *prev = NULL;
	ESP_LOGI(TAG, "adding timer to timerlist:");
	if (tmr == NULL)
	{
		ESP_LOGI(TAG, "new timer is null");
		return 0;
	}
	//若当前无定时器则，tmr将作为第一个定时器
	if (timer_list == NULL)
	{
		ESP_LOGI(TAG, "timer %s is the only timer", tmr->name);
		//first tmr
		tmr->next = NULL;
		timer_list = tmr;
	}
	else //按时间先后顺序将tmr插入链表
	{
		t = timer_list;
		//时间小的在前面
		while (((t->timeout.value) < (tmr->timeout.value)))
		{
			ESP_LOGI(TAG, "step for next timer");
			prev = t; //保存t的上一个
			if (t->next == NULL)
			{
				//若t是最后一个定时器
				t->next = tmr; //将tmr插入t后
				tmr->next = NULL;
				ESP_LOGI(TAG, "add timer %s to the tail of the list ", tmr->name);
				return 1;
			}
			t = t->next; //检查下一个定时器
		}
		//上一个为空，说明tmr是最早的定时器
		if (prev == NULL)
		{

			//tmr less than the first one
			tmr->next = timer_list; //tmr插入timer_list的前面
			timer_list = tmr;		//再更新timer_list
			ESP_LOGI(TAG, "add timer %s to the head of list", timer_list->name);
		}
		else
		{

			tmr->next = t; //tmr插入t之前

			prev->next = tmr; //tmr插入prev之后
			ESP_LOGI(TAG, "add timer %s behind %s", tmr->name, prev->name);
		}
	}
	return 1;
}
/*
 * 将定时器从链表移除
 * tmr：要移除的定时器
 * 返回 0失败；1成功
 */
int tmr_remove(struct timer *tmr)
{
	struct timer *t, *prev = NULL;
	if (tmr == NULL)
	{
		return 0;
	}

	//check if the tmr is in the list
	if (timer_list == NULL)
	{
		return 0;
	}
	t = timer_list;

	//find the tmr
	while (t != tmr)
	{
		if (t->next == NULL)
		{
			return 0; //no match
		}
		prev = t;
		t = t->next;
	}

	//t==tmr
	if (prev == NULL)
	{
		//t is the first
		timer_list = t->next;
	}
	else
	{
		if (t->next == NULL)
		{
			//t is the last
			prev->next = NULL;
		}
		else
		{
			//t in the middle
			prev->next = t->next;
		}
	}
	t->next = NULL;

	return 1;
}
/*
 * 删除定时器，释放定时器占用的内存
 * tmr：要删除的定时器
 * 返回：0失败，1成功
 */
int tmr_delete(struct timer *tmr)
{
	if (tmr == NULL)
	{
		return 0;
	}

	tmr->remove(tmr);

	free(tmr->name);

	free(tmr);

	return 1;
}
/*
 * 创建一个定时器 并添加进定时队列
 * conf：定时时间
 * cb：定时回调函数
 * arg：回调函数参数
 * name：定时器名
 * 返回：定时器结构体指针
 */
struct timer *tmr_new(clk_t *conf, timer_cb cb, void *arg, char *name)
{
	struct timer *tmr;
	clk_t clk;

	//tmr must > global_clk
	if (conf == NULL || (conf->value <= global_clk.value))
	{
		ESP_LOGI(TAG, "new tmr error\n");
		return 0;
	}
	tmr = (struct timer *)malloc(sizeof(struct timer));
	tmr->add = tmr_add;
	tmr->delete = tmr_delete;
	tmr->remove = remove;
	//chcek the conf

	clk.value = conf->value;
	save_clk(&clk); //规范化clk
	ESP_LOGI(TAG, "new timer %s: 20%d-%d-%d %d:%d:%d values =%u\r\n", name, clk.cal.year, clk.cal.month, clk.cal.date, clk.cal.hour, clk.cal.minute, clk.cal.second, clk.value);
	//printf("new timer %s: 20%d-%d-%d %d:%d:%d values =%u\r\n", name, clk.cal.year, clk.cal.month, clk.cal.date, clk.cal.hour, clk.cal.minute, clk.cal.second, clk.value);
	tmr->timeout.value = clk.value; //将clk赋值给定时器

	if (cb != NULL)
	{
		tmr->cb = cb;
	}
	if (arg != NULL)
	{
		tmr->arg = arg;
	}
	if (name != NULL)
	{
		char *dest = (char *)malloc(strlen(name));
		strncpy(dest, name, strlen(name) + 1);
		tmr->name = dest;
	}
	tmr->next = NULL;

	tmr->add(tmr);
	printf_tmrlist();
	return tmr;
}

void printf_tmrlist()
{
	if (timer_list == NULL)
	{
		return;
	}
	printf("timer list:\r\n");
	struct timer *t = timer_list;
	do
	{
		printf("%s: 20%d-%d-%d %d:%d:%d \r\n", t->name, t->timeout.cal.year, t->timeout.cal.month, t->timeout.cal.date, t->timeout.cal.hour, t->timeout.cal.minute, t->timeout.cal.second);

		//printf("%s value = %u\r\n", t->name, t->timeout.value);
		t = t->next;
	} while (t != NULL);
}




/*
 * 时间任务进程 负责更新全局时间以及触发定时器
 * 在定时器中断中，每秒执行
 */
void tmr_process(void *arg)
{

	struct timer *t;

	//update the global_clk
	global_clk.cal.second++;

	save_clk(&global_clk);

	//check if timer_list is null
	if (timer_list == NULL)
	{
		return;
	}
	//check the first timer in list
	t = timer_list;
	//printf("timer_list value = %d\n",t->timeout.value);
	if (global_clk.value == (t->timeout.value))
	{
		t->remove(t); //若要重复定时，只需要在回调函数中调用tmr_add()

		printf("timer %s\n", t->name);
		t->cb(t, t->arg); //callback func
	}
}

/*
 * 设置全局事件
 * conf：新的时间
 */
void tmr_set_global(clk_t conf)
{
	global_clk.value = conf.value;

	ESP_LOGI(TAG, "Current time: 20%d-%d-%d %d:%d:%d",global_clk.cal.year, global_clk.cal.month,global_clk.cal.date,global_clk.cal.hour,global_clk.cal.minute,global_clk.cal.second);
}

/*
 * 获取当前网络时间
 * 返回：时间结构体
 */
int get_current_nettime(clk_t *conf)
{
	char *origin = get_Time_String(); //调用myhttp.c,获取时间字符串："2021-03-24 04:49:53"
	if (origin == NULL)
	{
		printf("get network time err\n");
		return ESP_FAIL;
	}
	char str[6][10] = {'\0'}; //year,month,date,hour,minute,second
	//printf("origin = %s\n", origin);

	//分别提取出时间
	for (int i = 0; i <= 5; i++)
	{
		char *c = str[i];
		while (!((*origin == '-') || (*origin == ' ') || (*origin == ':')))
		{
			if (*origin == '\0')
			{
				break;
			}
			*c = *origin;
			origin++;
			c++;
		}
		origin++;
	}
	unsigned int x[6] = {0};
	//将字符串转换成数字
	for (int i = 0; i < 6; i++)
	{
		x[i] = (unsigned int)atoi(str[i]);
		//printf("x = %d\n", x[i]);
	}
	//赋值给时间结构体

	conf->cal.year = x[0] - 2000;
	conf->cal.month = x[1];
	conf->cal.date = x[2];
	conf->cal.hour = x[3] + 8;
	conf->cal.minute = x[4];
	conf->cal.second = x[5];
	return ESP_OK;
}

/*
 * 定时器回调函数：更新全局时间
 */
timer_cb update_global_cb(struct timer *tmr, void *agr)
{
	clk_t t;
	//无法更新时间
	while (get_current_nettime(&t) != ESP_OK)
	{
		vTaskDelay(100 / portTICK_RATE_MS);
	}
	tmr_set_global(t); //更新全局时间

	tmr->timeout.value = global_clk.value;
	tmr->timeout.cal.date += 1;

	tmr->add(tmr); //将更新任务再次添加进
	return NULL;
}




uint32_t get_clk_value()
{
	return global_clk.value;
}
void update_clk()
{
	xEventGroupSetBits(clk_event, UPDATE_CLK);
}

/*
 * 时钟任务 
 * 校准时间，维持时钟系统
 */
void clk_task()
{
	clk_t t;
	//初始化时间
	while (get_current_nettime(&t) != ESP_OK)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

	//get the time ok
	tmr_set_global(t); //更新全局时间

	t.cal.date += 1;
	tmr_new(&t, update_global_cb, NULL, "UPDATE");
	ESP_LOGI(TAG, "Clock Init OK");
	while (1)
	{
		//等待用户调用
		EventBits_t bit = xEventGroupWaitBits(clk_event, UPDATE_CLK | NEW_TIMER, pdTRUE, pdFALSE, 1000 / portTICK_RATE_MS);
		tmr_process(NULL);	//一秒到达，处理时钟事务

		//用户调用更新时间
		if (bit & UPDATE_CLK)
		{
			while (get_current_nettime(&t) != ESP_OK)
			{
				vTaskDelay(100 / portTICK_RATE_MS);
			}
			tmr_set_global(t); //更新全局时间
		}

		//用户添加定时器
		if (bit & NEW_TIMER)
		{
		}
	}
}

/*
 * 初始化时钟系统 创建时钟任务 设置事件组
 */
void clk_init()
{
	esp_log_level_set(TAG, ESP_LOG_INFO);
	ESP_LOGI(TAG, "init clk");
	clk_event = xEventGroupCreate();
	xEventGroupClearBits(clk_event, UPDATE_CLK | NEW_TIMER);

	xTaskCreate(clk_task, "clock_Task", CLK_TASK_SIZE, NULL, CLK_TASK_PRO, NULL);
}