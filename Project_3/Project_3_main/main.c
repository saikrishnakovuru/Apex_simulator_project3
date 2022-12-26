/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "apex_cpu.h"

int
main(int argc, char const *argv[])
{
    
    APEX_CPU *cpu;

    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", 0.0);

    if (argc <= 1 || argc >=7)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    cpu = APEX_cpu_init(argv[1]);
    

    if(argc > 2 && argc < 5)
    {
        //printf("\n\n argc <<<<5\n\n");
      const char* operation = argv[2];
      printf("%s\n",operation);
      printf("cycles=%s\n",argv[3]);
      int noofcycles = atoi(argv[3]);

      const char *ford = argv[3];
      char newford = *ford;

      //strncpy(newford,ford,0);
      //printf("\n\n newford = %s\n\n\n", ford);



        if (!cpu)
        {
            fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
            exit(1);
        }
        
        if(argc == 4)
        {
            if(strcmp(operation, "simulate") == 0)
            {
                printf("Inside simulate and cycles = %d\n",noofcycles);
                APEX_cpu_simulate(cpu, noofcycles);
                APEX_cpu_stop(cpu);
                return 0;
            }

            if(strcmp(operation, "display") == 0)
            {
                APEX_cpu_display(cpu, noofcycles, newford);
                APEX_cpu_stop(cpu);
                return 0;
            }

            if(strcmp(operation, "fwd") == 0)
            {
                //printf("\n\nHI\n\n");
                APEX_cpu_forward(cpu, newford);
                APEX_cpu_stop(cpu);
                return 0;
            }
        }

        
    }
    else if(argc >= 5)
    {
        //printf("\n\n argc >5\n\n");
        const char* operation = argv[2];
        printf("%s\n",operation);
        printf("cycles=%s\n",argv[3]);
        int noofcycles = atoi(argv[3]);

        const char *ford = argv[5];
        char newford = *ford;

        if(argc == 6)
        {
            //printf("\n\n\nHI\n\n\n");

            if(strcmp(operation, "display") == 0)
            {
                //cpu->forwarding = newford;
                //printf("\n\n\nnewford = %c\n\n\n", newford);
                APEX_cpu_display(cpu, noofcycles, newford);
                APEX_cpu_stop(cpu);
                return 0;
            }

            if(strcmp(operation, "show_mem") == 0)
            {
                //cpu->forwarding = newford;
                //printf("\n\n\nnewford = %c\n\n\n", newford);
                APEX_cpu_show_mem(cpu, noofcycles, newford);
                APEX_cpu_stop(cpu);
                return 0;
            }

        }
    }
    else
    {
        // printf("\n\nEnable Forwarding (Y/N) : \n\n");
        // scanf("%c", &forwarding);
        // printf("\n\n%c : Forwarding Enabled\n\n", forwarding);

        //cpu->forwarding = forwarding;

        APEX_cpu_run(cpu);
        APEX_cpu_stop(cpu);
        return 0;
    }
}