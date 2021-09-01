#include <stdio.h>
#include <stdlib.h>
int main()
{
	char *result="设置空调28度";
	char *p=result;
	char numstr[3]={0};
	char *p1=numstr;
	while(*p!='\0')
	{
		if(*p>47 && *p <58 && p1!=p1+3)
		{
			*p1=*p;
			p1++;
		}
		p++;
	}
	int num=atoi(numstr);
	printf("%d\n",num);
}
