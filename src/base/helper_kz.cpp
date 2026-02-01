// Copyright (C) Benjamín Gajardo (also known as +KZ)

#include "system.h"
#include <cmath>
#include "helper_kz.h"

const char * str_format_time_kz(double Time)
{
    static char stringvar[256];
	char tempseconds[16];
	double tempvar, floattime;
	floattime = ((int)Time) % 60 + std::modf(Time, &tempvar);
	if(Time >= 60.f) //+KZ minutes
	{
		if(Time / 60.f >= 60) //+KZ hours
		{
			if(floattime < 10)
			{
				get_str_double_kz(tempseconds, sizeof(tempseconds), floattime);
				str_format(stringvar, sizeof(stringvar), "%0d:%0d:0%s", (int)(Time / 60) / 60, (int)Time / 60, tempseconds);
			}
			else
			{
				get_str_double_kz(tempseconds, sizeof(tempseconds), floattime);
				str_format(stringvar, sizeof(stringvar), "%0d:%0d:%s", (int)(Time / 60) / 60, (int)Time / 60, tempseconds);
			}
		}
		else
		{
			if(floattime < 10)
			{
				get_str_double_kz(tempseconds, sizeof(tempseconds), floattime);
				str_format(stringvar, sizeof(stringvar), "%0d:0%s", (int)Time / 60, tempseconds);
			}
			else
			{
				get_str_double_kz(tempseconds, sizeof(tempseconds), floattime);
				str_format(stringvar, sizeof(stringvar), "%0d:%s", (int)Time / 60, tempseconds);
			}
		}
	}
	else
	{
		get_str_double_kz(tempseconds, sizeof(tempseconds), floattime);
		str_format(stringvar, sizeof(stringvar), "%s", tempseconds);
	}

    return stringvar;
}

void get_str_double_kz(char *buffer, int buffer_size, double value)
{
	if(snprintf(buffer, buffer_size, "%.2f", value) < 0)
		dbg_break();
}
