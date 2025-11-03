// Copyright (C) Benjamín Gajardo (also known as +KZ)

#include "system.h"
#include <cmath>

const char * str_format_time_kz(float Time)
{
    static char stringvar[256];
	float tempvar, floattime;
	floattime = ((int)Time) % 60 + std::modf(Time, &tempvar);
	if(Time >= 60.f) //+KZ minutes
	{
		if(Time / 60.f >= 60) //+KZ hours
		{
			if(floattime < 10)
				str_format(stringvar, sizeof(stringvar), "%0d:%0d:0%f", (int)(Time / 60) / 60, (int)Time / 60, floattime);
			else
				str_format(stringvar, sizeof(stringvar), "%0d:%0d:%f", (int)(Time / 60) / 60, (int)Time / 60, floattime);
		}
		else
		{
			if(floattime < 10)
				str_format(stringvar, sizeof(stringvar), "%0d:0%f", (int)Time / 60, floattime);
			else
				str_format(stringvar, sizeof(stringvar), "%0d:%f", (int)Time / 60, floattime);
		}
	}
	else
	{
		str_format(stringvar, sizeof(stringvar), "%f", floattime);
	}

    return stringvar;
}
