#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "../headers/tempo.h"
#include "../headers/protocolo.h"
#include "../headers/kbhit.h"

int main()
{
    if (OpenValve)
    {
        delta += value
    }
    if (CloseValve)
    {
        delta -= value
    }
    if (SetMax)
    {
        Max = value
    }
    if (delta > 0)
    {
        if (delta < 0.01 * dT)
        {
            in.angle(T + dT) = in.angle(T) + delta;
            delta = 0 else in.angle(T + dT) = in.angle(T) + 0.01 * dT;
            delta -= 0.01 * dT
        }
        else if (delta < 0)
        {
            if (delta > -0.01 * dT)
            {
                in.angle(T + dT) = in.angle(T) + delta;
                delta = 0 else in.angle(T + dT) = in.angle(T) - 0.01 * dT;
                delta += 0.01 * dT
            }
        }
    }
    in.angle(0) = 50;
    influx(T) = 1 * sin(pi / 2 * in.angle(T) / 100);
    outflux(T) = (MAX / 100) * (level(T) / 1.25 + 0.2) * sin(pi / 2 * outAngle(T) / 100);
    level(0) = 0.4;
    level(T + dT) = level(T) + 0.00002 * dT * (influx(T) - outflux(T));
}

float outAngle(long int T)
{
    if (T <= 0)
    {
        return 50;
    }
    if (T < 20000)
    {
        return (50 + T / 400);
    }
    if (T < 30000)
    {
        return 100;
    }
    if (T < 50000)
    {
        return (100 - (T - 30000) / 250);
    }
    if (T < 70000)
    {
        return (20 + (T - 50000) / 1000);
    }
    if (T < 100000)
    {
        return (40 + 20 * cos((T - 70000) * 2 * pi / 10000));
    }
    return 100;
}