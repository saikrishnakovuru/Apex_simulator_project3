/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "apex_cpu.h"
#include "apex_macros.h"



bool tempZflag = FALSE;
int tempBranch=0;
int branchTaken=0;
int inUseArray[15];
int count =0;
int inUsePrevious[15];
int previousR1b,previousR2b,previousR3b;
int temp=0;
int temp3=-1;
int rank = 0;
bool temp2=false;
int wbArray[30];
int presentCount=0;
int ifuStalled=0;
int ifuCount=0;
int takenBranches[8000];
int currentTakenBranch = 0;
// int physicalRegisters[15] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int currentPhysicalRegister = 0;

int resultBuffer[15] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

struct phyRegs {
    int dest;
    int pReg;
    int previousPR;
    bool takePrevious;
} phyRegs;

struct phyRegs physicalRegisters[15];
// void Insert(APEX_Instruction insertElement);
// void Delete(APEX_Instruction deleteElement);


CPU_Stage ROB[7];
int currentIndex = 0;
int sendingIndex = 0;




// int startPoint = 0;
// int endPoint = -1;


// void Insert(APEX_Instruction insertElement)
// {
//     // if(startPoint == -1)
//     // startPoint = 0;

//     // endPoint = endPoint + 1;
//     // ROB[endPoint] = insertElement;

//     printf("\n\nHere\n\n");

//     if(currentIndex == 4)
//     currentIndex = 0;

//     ROB[currentIndex] = insertElement;
//     currentIndex++;

// }

// void Delete(APEX_Instruction deleteElement)
// {
//     //startPoint = startPoint + 1;
// }







//int inUse =-500;
//bool R3 = false;
//bool R5 = false;
//int stop = 0;

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
        
        case OPCODE_SUB:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MUL:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_DIV:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_AND:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_OR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_ADDL:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_SUBL:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_BZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_BNZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_JUMP:
        {
            printf("%s, R%d, #%d ", stage->opcode_str, stage->rs1, stage->imm);
            break;
        }

        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_LDR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_STR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->rs3);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d,R%d", stage->opcode_str, stage->rd, stage->rs1, stage->rs2);
            break;
        }

    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE ; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

   
}

/* Debug function which prints the memory file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_mem_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Memory:");

    for (int i = 0; i < DATA_MEMORY_SIZE/2; ++i)
    {
        printf("M%-2d[%d] ", i, cpu->data_memory[i]);
    }

    printf("\n");

    for (i = (DATA_MEMORY_SIZE / 2); i < DATA_MEMORY_SIZE; ++i)
    {
        printf("M%-3d[%d] ", i, cpu->data_memory[i]);
    }

    printf("\n");
}

/* Debug function which prints the physical reg file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_phy_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Physical Registers:");

    for (int i = 0; i < PHY_REG_FILE_SIZE / 2; ++i)
    {
        printf("P%-3d[%-3d] ", i, cpu->phy_regs[i]);
    }

    printf("\n");

    for (i = (PHY_REG_FILE_SIZE / 2); i < PHY_REG_FILE_SIZE; ++i)
    {
        printf("P%-3d[%-3d] ", i, cpu->phy_regs[i]);
    }

    printf("\n");
}


/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;
    
    if(branchTaken == 1)
    cpu->fetch.has_insn = FALSE;

    if (cpu->fetch.has_insn)
    {

        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;
            printf("Fetch          : EMPTY\n");
        
            /* Skip this cycle*/
            return;
        }

        

        /* Store current PC in fetch latch */
        // cpu->fetch.pc = cpu->pc;
        //printf("\n\nbranchTaken = %d\n\n", branchTaken);
        
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.rs3 = current_ins->rs3;
        cpu->fetch.imm = current_ins->imm;

        cpu->fetch.currentPReg = -1;

        /* Update PC for next instruction */
        //cpu->pc += 4;

        if(cpu->fetch.opcode == OPCODE_BZ || cpu->fetch.opcode == OPCODE_BNZ)
        {
            //printf("\n\ncpu->fetch.imm = %d\n\n", cpu->fetch.imm);

            //printf("\n\nHere000000\n\n");

            if(cpu->fetch.imm < 0)
            {
                
                //printf("\n\nHere111\n\n");

                if(cpu->fetch.pc != takenBranches[cpu->fetch.pc])
                {
                    //printf("\n\nHere222222\n\n");
                    branchTaken = 1;
                    //break;
                }
                else
                {
                    cpu->fetch.bTaken = TRUE;
                    cpu->pc = cpu->fetch.pc + cpu->fetch.imm - 4;
                    

                    //cpu->fetch_from_next_cycle = TRUE;


                    //cpu->fetch.has_insn = TRUE;

                    //break;
                }

                
                

            }
            else if(cpu->fetch.imm < 0)
            {
                branchTaken = 1;
            }
            else
            {
                //branchTaken = 0;
            }
        }
        else if(cpu->fetch.opcode == OPCODE_JUMP)
        {
            branchTaken = 1;
        }
        else
        {
            //branchTaken = 0;
        }


        if(cpu->decode.isStalled != TRUE)
        {
            cpu->pc += 4;
            cpu->decode = cpu->fetch;
        }

        // if(branchTaken == 1)
        // {
        //     //cpu->fetch_from_next_cycle = TRUE;
            
        // }
        // else
        // {
        //     //cpu->fetch_from_next_cycle = FALSE;

        // }

        // if(branchTaken != 0)
        // cpu->fetch.has_insn=FALSE;

        // else
        // {
        //     //cpu->decode.has_insn = FALSE;
        //     //cpu->fetch.isStalled = TRUE;
        // }


        /* Copy data from fetch latch to decode latch*/
        //cpu->decode = cpu->fetch;

        // if(branchTaken == 1)
        // cpu->fetch.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->fetch.opcode != OPCODE_HALT)
        {
            //printf("\nBefore fetch: \n cpu->fetch.rd : %d inUseArray[cpu->fetch.rd] : %d cpu->fetch.rs1 :%d inUseArray[cpu->fetch.rs1] : %d cpu->fetch.rs2 : %d inUseArray[cpu->fetch.rs2] :%d cpu->fetch.rs3 : %d inUseArray[cpu->fetch.rs3] :%d\n \n", cpu->fetch.rd, inUseArray[cpu->fetch.rd], cpu->fetch.rs1, inUseArray[cpu->fetch.rs1], cpu->fetch.rs2, inUseArray[cpu->fetch.rs2], cpu->fetch.rs3, inUseArray[cpu->fetch.rs3]);
            // if(branchTaken == 0)
            // print_stage_content("Fetch", &cpu->fetch);
            // else
            // printf("Fetch          : EMPTY\n");

            print_stage_content("Fetch", &cpu->fetch);
        }
        else if(ENABLE_DEBUG_MESSAGES && cpu->fetch.opcode == OPCODE_HALT)
        {
            if(cpu->fetch.opcode == OPCODE_HALT)
            print_stage_content("Fetch", &cpu->fetch);
            else
            printf("Fetch          : EMPTY\n");
        }
        else
        {
            printf("Fetch          : EMPTY\n");
        }
        

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->decode.isStalled != TRUE && cpu->fetch.opcode == OPCODE_HALT)
        {
            //printf("Fetch      :EMPTY");
            //printf("Fetch          :EMPTY\n");
            cpu->decode = cpu->fetch;
            cpu->fetch.has_insn = FALSE;
        }
        
    }
    else if(cpu->fetch.opcode != OPCODE_HALT && cpu->decode.isStalled != TRUE)
    {
        printf("Fetch          : EMPTY\n");
        cpu->decode = cpu->fetch;
    }
    else if(cpu->fetch.opcode == OPCODE_HALT)
    {
        printf("Fetch          : EMPTY\n");
        //cpu->decode = cpu->fetch;
    }
    else
    {
        cpu->fetch.has_insn = FALSE;
        printf("Fetch          : EMPTY\n");
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    
    //if (cpu->decode.has_insn && cpu->decode.isStalled == FALSE)
    if (cpu->decode.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            
            case OPCODE_ADD:
            {
                inUseArray[cpu->decode.rd] = 1;

                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_ADDL:
            {
                inUseArray[cpu->decode.rd] = 1;

                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_SUB:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_SUBL:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_MUL:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_DIV:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_AND:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_OR:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_XOR:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_LOAD:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_STORE:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                //cpu->decode.imm = cpu->regs[cpu->decode.imm];
                break;
            }

            case OPCODE_MOVC:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                /* MOVC doesn't have register operands */
                break;
            }

            case OPCODE_BZ:
            {
                /* MOVC doesn't have register operands */
                break;
            }

            case OPCODE_BNZ:
            {
                /* MOVC doesn't have register operands */
                break;
            }

            case OPCODE_JUMP:
            {                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_HALT:
            {
                /* MOVC doesn't have register operands */
                break;
            }

            case OPCODE_NOP:
            {
                /* MOVC doesn't have register operands */
                break;
            }

            case OPCODE_LDR:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_STR:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                cpu->decode.rs3_value = cpu->regs[cpu->decode.rs3];
                break;
            }

            case OPCODE_CMP:
            {
                inUseArray[cpu->decode.rd] = 1;
                
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                
                break;
            }
        }



    
       
       
        // if(inUseArray[cpu->decode.rs1] == 1 || inUseArray[cpu->decode.rs2] == 1  || inUseArray[cpu->decode.rs3] == 1)
        // {



        //     if(cpu->forwarding == TRUE)
        //     {
        //         if(inUseArray[cpu->decode.rs1] == 1 && inUseArray[cpu->decode.rs2] == 1 && inUseArray[cpu->decode.rs3] == 1) 
        //         {
        //             //inUsePrevious[cpu->decode.rs1] = cpu->decode.rs1_value;
        //             if(resultBuffer[cpu->decode.rs1] == -1 && resultBuffer[cpu->decode.rs2] == -1 && resultBuffer[cpu->decode.rs3] == -1)
        //             {
        //                 cpu->decode.isStalled = TRUE;
        //                 cpu->fetch.isStalled = TRUE;
        //             }
        //             else
        //             {
        //                 cpu->decode.rs1_value = resultBuffer[cpu->decode.rs1];
        //                 cpu->decode.rs2_value = resultBuffer[cpu->decode.rs2];
        //                 cpu->decode.rs3_value = resultBuffer[cpu->decode.rs3];
        //                 resultBuffer[cpu->decode.rs1] = -1;
        //                 resultBuffer[cpu->decode.rs2] = -1;
        //                 resultBuffer[cpu->decode.rs3] = -1;
        //             }
        //         }
        //         else if(inUseArray[cpu->decode.rs1] == 1 && inUseArray[cpu->decode.rs2] == 1) 
        //         {
        //             //inUsePrevious[cpu->decode.rs1] = cpu->decode.rs1_value;

        //             if(resultBuffer[cpu->decode.rs1] == -1 && resultBuffer[cpu->decode.rs2] == -1)
        //             {
        //                 cpu->decode.isStalled = TRUE;
        //                 cpu->fetch.isStalled = TRUE;
        //             }
        //             else
        //             {
        //                 //printf("\n\nHi\n\n");
        //                 cpu->decode.rs1_value = resultBuffer[cpu->decode.rs1];
        //                 cpu->decode.rs2_value = resultBuffer[cpu->decode.rs2];

        //                 resultBuffer[cpu->decode.rs1] = -1;
        //                 resultBuffer[cpu->decode.rs2] = -1;
        //             }
        //         }
        //         else if(inUseArray[cpu->decode.rs2] == 1 && inUseArray[cpu->decode.rs3] == 1) 
        //         {
        //             //inUsePrevious[cpu->decode.rs1] = cpu->decode.rs1_value;
        //             if(resultBuffer[cpu->decode.rs2] == -1 && resultBuffer[cpu->decode.rs3] == -1)
        //             {
        //                 cpu->decode.isStalled = TRUE;
        //                 cpu->fetch.isStalled = TRUE;
        //             }
        //             else
        //             {
        //                 cpu->decode.rs2_value = resultBuffer[cpu->decode.rs2];
        //                 cpu->decode.rs3_value = resultBuffer[cpu->decode.rs3];
                        
        //                 resultBuffer[cpu->decode.rs2] = -1;
        //                 resultBuffer[cpu->decode.rs3] = -1;
        //             }
        //         }
        //         else if(inUseArray[cpu->decode.rs1] == 1 && inUseArray[cpu->decode.rs3] == 1) 
        //         {
        //             //inUsePrevious[cpu->decode.rs1] = cpu->decode.rs1_value;

        //             if(resultBuffer[cpu->decode.rs1] == -1 && resultBuffer[cpu->decode.rs3] == -1)
        //             {
        //                 cpu->decode.isStalled = TRUE;
        //                 cpu->fetch.isStalled = TRUE;
        //             }
        //             else
        //             {
        //                 cpu->decode.rs1_value = resultBuffer[cpu->decode.rs1];
        //                 cpu->decode.rs3_value = resultBuffer[cpu->decode.rs3];

        //                 resultBuffer[cpu->decode.rs1] = -1;
        //                 resultBuffer[cpu->decode.rs3] = -1;
        //             }
                    
        //         }
        //         else if(inUseArray[cpu->decode.rs1] == 1) 
        //         {
        //             //inUsePrevious[cpu->decode.rs1] = cpu->decode.rs1_value;

        //             //cpu->decode.rs1_value = resultBuffer[cpu->decode.rs1];

        //             //printf("\n\n\nHI\n\n\n");
        //             //printf("\n\n\n\n HI im here 2\n\n\n\n");

        //             if(resultBuffer[cpu->decode.rs1] == -1)
        //             {
        //                 cpu->decode.isStalled = TRUE;
        //                 cpu->fetch.isStalled = TRUE;
        //             }
        //             else
        //             {
        //                 cpu->decode.rs1_value = resultBuffer[cpu->decode.rs1];
        //                 resultBuffer[cpu->decode.rs1] = -1;
        //             }

        //         }
        //         else if (inUseArray[cpu->decode.rs2] == 1)
        //         {
        //             //inUsePrevious[cpu->decode.rs2] = cpu->decode.rs2_value;

        //             //cpu->decode.rs2_value = resultBuffer[cpu->decode.rs2];

        //             //printf("\n\n\nHI - 2\n\n\n");

        //             if(resultBuffer[cpu->decode.rs2] == -1)
        //             {
        //                 cpu->decode.isStalled = TRUE;
        //                 cpu->fetch.isStalled = TRUE;
        //             }
        //             else
        //             {
        //                 cpu->decode.rs2_value = resultBuffer[cpu->decode.rs2];
        //                 resultBuffer[cpu->decode.rs2] = -1;
        //             }
        //         }
        //         else if (inUseArray[cpu->decode.rs3] == 1)
        //         {
        //             //inUsePrevious[cpu->decode.rs3] = cpu->decode.rs3_value;
        //             if(resultBuffer[cpu->decode.rs3] == -1)
        //             {
        //                 cpu->decode.isStalled = TRUE;
        //                 cpu->fetch.isStalled = TRUE;
        //             }
        //             else
        //             {
        //                 cpu->decode.rs3_value = resultBuffer[cpu->decode.rs3];
        //                 resultBuffer[cpu->decode.rs3] = -1;
        //             }
        //         }
        //         else
        //         {
        //             // cpu->decode.rs3_value = resultBuffer[cpu->decode.rs3];
        //             // resultBuffer[cpu->decode.rs3] = -1;
        //         }

        //     }
        //     else
        //     {
        //         cpu->decode.isStalled = TRUE;
        //         cpu->fetch.isStalled = TRUE;
        //     }
        // }
            



        //printf("\nBefore : \n cpu->decode.rd : %d inUseArray[cpu->decode.rd] : %d cpu->decode.rs1 :%d inUseArray[cpu->decode.rs1] : %d cpu->decode.rs2 : %d inUseArray[cpu->decode.rs2] :%d cpu->decode.rs3 : %d inUseArray[cpu->decode.rs3] :%d\n \n", cpu->decode.rd, inUseArray[cpu->decode.rd], cpu->decode.rs1, inUseArray[cpu->decode.rs1], cpu->decode.rs2, inUseArray[cpu->decode.rs2], cpu->decode.rs3, inUseArray[cpu->decode.rs3]);

        //printf("\n\ncount : %d\n\n", count);

        // if(count == 0)
        // {
            

        //     //printf("\n\n\n Hi - 3\n\n\n");

        //     if(cpu->decode.opcode == OPCODE_ADD || cpu->decode.opcode == OPCODE_SUB || cpu->decode.opcode == OPCODE_MUL || cpu->decode.opcode == OPCODE_DIV
        //      || cpu->decode.opcode == OPCODE_LDR || cpu->decode.opcode == OPCODE_AND || cpu->decode.opcode == OPCODE_OR || cpu->decode.opcode == OPCODE_XOR)
        //     {
        //         //inUseArray[cpu->decode.rd] = 0;
        //         inUseArray[cpu->decode.rd] = 1;
        //         //inUseArray[cpu->decode.rs1] = 1;
        //         //inUseArray[cpu->decode.rs2] = 1;
        //     }
        //     else if(cpu->decode.opcode == OPCODE_CMP)
        //     {
        //         inUseArray[cpu->decode.rd] = 1;
        //         //inUseArray[cpu->decode.rs2] = 1;
        //     }
        //     else if(cpu->decode.opcode == OPCODE_STORE)
        //     {
        //         //inUseArray[cpu->decode.rs1] = 1;
        //         //inUseArray[cpu->decode.rs2] = 1;
        //     }
        //     else if(cpu->decode.opcode == OPCODE_ADDL || cpu->decode.opcode == OPCODE_SUBL || cpu->decode.opcode == OPCODE_LOAD)
        //     {
        //         inUseArray[cpu->decode.rd] = 1;
        //         //inUseArray[cpu->decode.rs1] = 1;
        //     }
        //     else if(cpu->decode.opcode == OPCODE_MOVC)
        //     {
                
        //         inUseArray[cpu->decode.rd] = 1;

        //         //printf("\n\n\n Hi - 4 :    inUseArray[cpu->decode.rd] = %d\n\n\n", inUseArray[cpu->decode.rd]);
        //     }
        //     else if(cpu->decode.opcode == OPCODE_STR)
        //     {
        //         inUseArray[cpu->decode.rd] = 1;
        //         //inUseArray[cpu->decode.rs2] = 1;
        //         //inUseArray[cpu->decode.rs3] = 1;
        //     }
        //     else if(cpu->decode.opcode == OPCODE_BNZ || cpu->decode.opcode == OPCODE_BZ)
        //     {
        //         // inUseArray[cpu->decode.rs1] = 0;
        //         // inUseArray[cpu->decode.rs2] = 0;
        //         // inUseArray[cpu->decode.rs3] = 0;
        //     }
        //     else
        //     {
        //         inUseArray[cpu->decode.rd] = 1;
        //     }
        // }
        // else
        // {
        //     if(cpu->decode.isStalled != TRUE || cpu->fetch.isStalled != TRUE)
        //     {

        //         //printf("\n\nHi\n\n");
                

        //         if(cpu->decode.opcode == OPCODE_ADD || cpu->decode.opcode == OPCODE_SUB || cpu->decode.opcode == OPCODE_MUL || cpu->decode.opcode == OPCODE_DIV
        //         || cpu->decode.opcode == OPCODE_LDR || cpu->decode.opcode == OPCODE_AND || cpu->decode.opcode == OPCODE_OR || cpu->decode.opcode == OPCODE_XOR)
        //         {
        //             //inUseArray[cpu->decode.rd] = 0;
        //             inUseArray[cpu->decode.rd] = 1;
        //             //inUseArray[cpu->decode.rs1] = 1;
        //             //inUseArray[cpu->decode.rs2] = 1;
        //         }
        //         else if(cpu->decode.opcode == OPCODE_CMP)
        //         {
        //             inUseArray[cpu->decode.rd] = 1;
        //             //inUseArray[cpu->decode.rs2] = 1;
        //         }
        //         else if(cpu->decode.opcode == OPCODE_STORE)
        //         {
        //             //inUseArray[cpu->decode.rs1] = 1;
        //             //inUseArray[cpu->decode.rs2] = 1;
        //         }
        //         else if(cpu->decode.opcode == OPCODE_ADDL || cpu->decode.opcode == OPCODE_SUBL || cpu->decode.opcode == OPCODE_LOAD)
        //         {
        //             inUseArray[cpu->decode.rd] = 1;
        //             //inUseArray[cpu->decode.rs1] = 1;
        //         }
        //         else if(cpu->decode.opcode == OPCODE_MOVC)
        //         {
        //             inUseArray[cpu->decode.rd] = 1;
        //         }
        //         else if(cpu->decode.opcode == OPCODE_STR)
        //         {
        //             // inUseArray[cpu->decode.rs1] = 1;
        //             // inUseArray[cpu->decode.rs2] = 1;
        //             // inUseArray[cpu->decode.rs3] = 1;
        //         }
        //         else if(cpu->decode.opcode == OPCODE_BNZ || cpu->decode.opcode == OPCODE_BZ)
        //         {
        //             // inUseArray[cpu->decode.rs1] = 0;
        //             // inUseArray[cpu->decode.rs2] = 0;
        //             // inUseArray[cpu->decode.rs3] = 0;
        //         }
        //         else
        //         {
        //             inUseArray[cpu->decode.rd] = 1;
        //         }
                
        //     }
        // }

        // count++;


        // if(cpu->decode.isStalled != TRUE && (cpu->ifu.isStalled != TRUE || cpu->mul.isStalled != TRUE || cpu->lsfu.isStalled != TRUE))
        // //if(cpu->decode.isStalled != TRUE)
        // {
        //     //printf("\n\nHI\n\n");
        //     switch(cpu->decode.opcode)
        //     {
        //         // case OPCODE_STORE:
        //         // {
        //         //     cpu->lsfu = cpu->decode;
        //         //     cpu->lsfu.lsStalled = 4;
        //         //     //cpu->whichExecute = 3;
        //         //     break;
        //         // }

        //         // case OPCODE_LOAD:
        //         // {
        //         //     cpu->lsfu = cpu->decode;
        //         //     cpu->lsfu.lsStalled = 4;
        //         //     //cpu->whichExecute = 3;
        //         //     break;
        //         // }

        //         // case OPCODE_STR:
        //         // {
        //         //     cpu->lsfu = cpu->decode;
        //         //     cpu->lsfu.lsStalled = 4;
        //         //     //cpu->whichExecute = 3;
        //         //     break;
        //         // }

        //         case OPCODE_AND:
        //         {
        //             cpu->lsfu = cpu->decode;
        //             cpu->lsfu.lsStalled = 1;
        //             //cpu->whichExecute = 2;
        //             break;
        //         }

        //         case OPCODE_OR:
        //         {
        //             cpu->lsfu = cpu->decode;
        //             cpu->lsfu.lsStalled = 1;
        //             //cpu->whichExecute = 2;
        //             break;
        //         }

        //         case OPCODE_XOR:
        //         {
        //             cpu->lsfu = cpu->decode;
        //             cpu->lsfu.lsStalled = 1;
        //             //cpu->whichExecute = 2;
        //             break;
        //         }

        //         case OPCODE_MUL:
        //         {
        //             cpu->mul = cpu->decode;
        //             cpu->mul.mulStalled = 3;
        //             ifuStalled = 3;
        //             //cpu->ifu.mulStalled = 3;
        //             //cpu->whichExecute = 2;
        //             break;
        //         }

        //         default:
        //         {
        //             cpu->ifu = cpu->decode;
        //             //cpu->whichExecute = 1;
        //             break;
        //         }
        //     }

        //     //cpu->execute = cpu->decode;

        //     if (ENABLE_DEBUG_MESSAGES && cpu->decode.opcode != OPCODE_HALT)
        //     print_stage_content("Decode/RF", &cpu->decode);
            
        // }

        //printf("\nAfter : \n cpu->decode.rd : %d inUseArray[cpu->decode.rd] : %d cpu->decode.rs1 :%d inUseArray[cpu->decode.rs1] : %d cpu->decode.rs2 : %d inUseArray[cpu->decode.rs2] :%d\n \n", cpu->decode.rd, inUseArray[cpu->decode.rd], cpu->decode.rs1, inUseArray[cpu->decode.rs1], cpu->decode.rs2, inUseArray[cpu->decode.rs2]);

        






        /* Copy data from decode latch to execute latch*/
        // if(cpu->decode.isStalled != TRUE && cpu->execute.isStalled != TRUE)
        // {
        //     cpu->execute = cpu->decode;

        //     if (ENABLE_DEBUG_MESSAGES && cpu->decode.opcode != OPCODE_HALT)
        //     print_stage_content("Decode/RF", &cpu->decode);
        //     else
        //     printf("Decode/RF      : EMPTY\n");
            
        // }
        //else
        //{

            cpu->dispatch = cpu->decode;
            
            
            if (ENABLE_DEBUG_MESSAGES)
            {
                if(cpu->decode.has_insn == FALSE)
                printf("Decode/RF      : EMPTY\n");
                else
                {
                    print_stage_content("Decode/RF", &cpu->decode);
                    cpu->decode.has_insn = FALSE;
                }
            }
            else
            printf("Decode/RF      : EMPTY\n");

        //}


        //cpu->execute = cpu->decode;
        //cpu->decode.has_insn = FALSE;

        // if (ENABLE_DEBUG_MESSAGES)
        // {
        //     print_stage_content("Decode/RF", &cpu->decode);
        // }
    }
    else if(cpu->decode.opcode == OPCODE_HALT)
    {
        printf("DECODE         : EMPTY\n");
        //cpu->decode = cpu->fetch;
    }
    else
    {
        if(ENABLE_DEBUG_MESSAGES)
        {
            if(cpu->decode.has_insn == FALSE || cpu->decode.opcode == OPCODE_HALT)
            {
                printf("Decode/RF      : EMPTY\n");
            }
            else
            {
                print_stage_content("Decode/RF", &cpu->decode);
                cpu->decode.has_insn = FALSE;
            }
        }
    }
}

static void
APEX_dispatch(APEX_CPU *cpu)
{
    if(cpu->dispatch.has_insn)
    {

        // if(physicalRegisters[currentPhysicalRegister] == 1)
        // currentPhysicalRegister++;

        // printf("\n\nCurrent Physical Reg = %d\n\n", currentPhysicalRegister);


        // if(currentPhysicalRegister == 14)
        // currentPhysicalRegister = 0;

        // //if(cpu->dispatch.rd != -1 && cpu->dispatch.opcode != OPCODE_CMP)
        // if(cpu->dispatch.rd != -1)
        // {
        //     cpu->dispatch.currentPReg = currentPhysicalRegister;
        //     //physicalRegisters[currentPhysicalRegister] = 1;
            
        //     physicalRegisters[currentPhysicalRegister].dest = cpu->dispatch.rd;
        //     physicalRegisters[currentPhysicalRegister].pReg = currentPhysicalRegister;

        //     currentPhysicalRegister++;
        // }
        

        



        cpu->ROB = cpu->dispatch;
        if(cpu->dispatch.opcode == OPCODE_LOAD || cpu->dispatch.opcode == OPCODE_STORE || cpu->dispatch.opcode == OPCODE_LDR || cpu->dispatch.opcode == OPCODE_STR)
        {
            cpu->LSQ = cpu->dispatch;
        }
        else
        {
            cpu->IQ = cpu->dispatch;
        }

    
        // if(cpu->dispatch.opcode == OPCODE_JUMP)
        // {
        //     branchTaken = 0;
        // }

        

        if (ENABLE_DEBUG_MESSAGES && cpu->dispatch.opcode != OPCODE_HALT)
        {
            print_stage_content("DISPATCH", &cpu->dispatch);
            cpu->dispatch.has_insn = FALSE;
            // if(cpu->dispatch.has_insn == FALSE || cpu->dispatch.opcode == OPCODE_HALT)
            // printf("DISPATCH       : EMPTY\n");
            // else
            // {
            //     print_stage_content("DISPATCH", &cpu->dispatch);
            //     cpu->dispatch.has_insn = FALSE;
            // }
        }
        else if(ENABLE_DEBUG_MESSAGES && cpu->dispatch.opcode == OPCODE_HALT)
        {
            if(cpu->dispatch.opcode == OPCODE_HALT)
            {
                print_stage_content("DISPATCH", &cpu->dispatch);
                cpu->dispatch.has_insn = FALSE;
            }
            else
            printf("DISPATCH       : EMPTY\n");
        }
        else
        printf("DISPATCH       : EMPTY\n");

        // if (cpu->dispatch.has_insn && cpu->dispatch.isStalled == FALSE)
        // {
        //     if (ENABLE_DEBUG_MESSAGES && cpu->dispatch.opcode != OPCODE_HALT)
        //     {
        //         print_stage_content("DISPATCH", &cpu->dispatch);
        //     }
        // }
        // else{
        //     if(ENABLE_DEBUG_MESSAGES)
        //     printf("DISPATCH       : EMPTY\n");
        // }

    }
    else if(cpu->dispatch.opcode == OPCODE_HALT)
    {
        printf("DISPATCH       : EMPTY\n");
        //cpu->decode = cpu->fetch;
    }
    else
    {
        if(ENABLE_DEBUG_MESSAGES)
        {
            if(cpu->dispatch.has_insn == FALSE || cpu->dispatch.opcode == OPCODE_HALT)
            {
                printf("DISPATCH       : EMPTY\n");
            }
            else
            print_stage_content("DISPATCH", &cpu->dispatch);
        }
    }
    

}

static void
APEX_IQ(APEX_CPU *cpu)
{
    if(cpu->IQ.has_insn)
    {
        //cpu->ROB = cpu->IQ;

        //Forwarding Starts

        //printf("\n\ninUseArray[cpu->IQ.rs1] == %d ; inUseArray[cpu->IQ.rs2] == %d ; inUseArray[cpu->IQ.rs3] == %d\n\n", inUseArray[cpu->IQ.rs1], inUseArray[cpu->IQ.rs2], inUseArray[cpu->IQ.rs3]);

        if(inUseArray[cpu->IQ.rs1] == 1 && inUseArray[cpu->IQ.rs2] == 1 && inUseArray[cpu->IQ.rs3] == 1) 
        {
            
                cpu->IQ.rs1_value = resultBuffer[cpu->IQ.rs1];
                cpu->IQ.rs2_value = resultBuffer[cpu->IQ.rs2];
                cpu->IQ.rs3_value = resultBuffer[cpu->IQ.rs3];
                //resultBuffer[cpu->IQ.rs1] = -1;
                //resultBuffer[cpu->IQ.rs2] = -1;
                //resultBuffer[cpu->IQ.rs3] = -1;
            
        }
        else if(inUseArray[cpu->IQ.rs1] == 1 && inUseArray[cpu->IQ.rs2] == 1) 
        {
            
                //printf("\n\nHi\n\n");
                cpu->IQ.rs1_value = resultBuffer[cpu->IQ.rs1];
                cpu->IQ.rs2_value = resultBuffer[cpu->IQ.rs2];

                //resultBuffer[cpu->IQ.rs1] = -1;
                //resultBuffer[cpu->IQ.rs2] = -1;
            
        }
        else if(inUseArray[cpu->IQ.rs2] == 1 && inUseArray[cpu->IQ.rs3] == 1) 
        {
            
                cpu->IQ.rs2_value = resultBuffer[cpu->IQ.rs2];
                cpu->IQ.rs3_value = resultBuffer[cpu->IQ.rs3];
                
                //resultBuffer[cpu->IQ.rs2] = -1;
                //resultBuffer[cpu->IQ.rs3] = -1;
            
        }
        else if(inUseArray[cpu->IQ.rs1] == 1 && inUseArray[cpu->IQ.rs3] == 1) 
        {
            
                cpu->IQ.rs1_value = resultBuffer[cpu->IQ.rs1];
                cpu->IQ.rs3_value = resultBuffer[cpu->IQ.rs3];

                //resultBuffer[cpu->IQ.rs1] = -1;
                //resultBuffer[cpu->IQ.rs3] = -1;
            
            
        }
        else if(inUseArray[cpu->IQ.rs1] == 1) 
        {
            
                cpu->IQ.rs1_value = resultBuffer[cpu->IQ.rs1];
                //resultBuffer[cpu->IQ.rs1] = -1;
            

        }
        else if (inUseArray[cpu->IQ.rs2] == 1)
        {
            
                cpu->IQ.rs2_value = resultBuffer[cpu->IQ.rs2];
                //resultBuffer[cpu->IQ.rs2] = -1;
            
        }
        else if (inUseArray[cpu->IQ.rs3] == 1)
        {
            
                cpu->IQ.rs3_value = resultBuffer[cpu->IQ.rs3];
                //resultBuffer[cpu->IQ.rs3] = -1;
            
        }
        else
        {
            
        }


        //Forwarding Ends


        if(cpu->IQ.opcode == OPCODE_AND || cpu->IQ.opcode == OPCODE_OR || cpu->IQ.opcode == OPCODE_XOR)
        {
            cpu->lsfu = cpu->IQ;
        }
        else if(cpu->IQ.opcode == OPCODE_MUL)
        {
            cpu->mul1 = cpu->IQ;
        }
        else
        {
            cpu->ifu = cpu->IQ;
        }

        if(cpu->ifu.opcode == OPCODE_JUMP || cpu->ifu.opcode == OPCODE_BZ || cpu->ifu.opcode == OPCODE_BNZ)
        {
            branchTaken = 0;
            //printf("\n\nHIII\n\n");
        }

        //cpu->ifu = cpu->IQ;

        // if (cpu->IQ.has_insn && cpu->IQ.isStalled == FALSE)
        // {
        //     if (ENABLE_DEBUG_MESSAGES && cpu->IQ.opcode != OPCODE_HALT)
        //     {
        //         print_stage_content("IQ", &cpu->IQ);
        //     }
        // }
        // else{
        //     if(ENABLE_DEBUG_MESSAGES)
        //     printf("IQ             : EMPTY\n");
        // }
        
        if (ENABLE_DEBUG_MESSAGES && cpu->IQ.opcode != OPCODE_HALT)
        {
            print_stage_content("IQ", &cpu->IQ);
            cpu->IQ.has_insn = FALSE;

            // if(cpu->IQ.has_insn == FALSE || cpu->IQ.opcode == OPCODE_HALT)
            // printf("IQ             : EMPTY\n\n");
            // else
            // {
            //     print_stage_content("IQ", &cpu->IQ);
            //     printf("\n");
            //     cpu->IQ.has_insn = FALSE;
            // }
        }
        else if(ENABLE_DEBUG_MESSAGES && cpu->IQ.opcode == OPCODE_HALT)
        {
            if(cpu->IQ.opcode == OPCODE_HALT)
            print_stage_content("IQ", &cpu->IQ);
            else
            printf("IQ          : EMPTY\n");
        }
        else
        {
            printf("IQ          : EMPTY\n");
        }
    }
    else if(cpu->IQ.opcode != OPCODE_HALT)
    {
        printf("IQ             : EMPTY\n");
    }
    else if(cpu->IQ.opcode == OPCODE_HALT)
    {
        printf("IQ          : EMPTY\n");

    }
    else
    {
        printf("IQ          : EMPTY\n");
        // if(ENABLE_DEBUG_MESSAGES)
        // {
        //     if(cpu->IQ.has_insn == FALSE || cpu->IQ.opcode == OPCODE_HALT)
        //     {
        //         printf("IQ             : EMPTY\n");
        //     }
        //     else
        //     print_stage_content("IQ", &cpu->IQ);
        // }
    }
}

static void
APEX_LSQ(APEX_CPU *cpu)
{
    if(cpu->LSQ.has_insn)
    {

        //Forwarding Starts

        //printf("\n\ninUseArray[cpu->LSQ.rs1] == %d ; inUseArray[cpu->LSQ.rs2] == %d ; inUseArray[cpu->LSQ.rs3] == %d\n\n", inUseArray[cpu->LSQ.rs1], inUseArray[cpu->LSQ.rs2], inUseArray[cpu->LSQ.rs3]);

        if(inUseArray[cpu->LSQ.rs1] == 1 && inUseArray[cpu->LSQ.rs2] == 1 && inUseArray[cpu->LSQ.rs3] == 1) 
        {
            
                cpu->LSQ.rs1_value = resultBuffer[cpu->LSQ.rs1];
                cpu->LSQ.rs2_value = resultBuffer[cpu->LSQ.rs2];
                cpu->LSQ.rs3_value = resultBuffer[cpu->LSQ.rs3];
                //resultBuffer[cpu->LSQ.rs1] = -1;
                //resultBuffer[cpu->LSQ.rs2] = -1;
                //resultBuffer[cpu->LSQ.rs3] = -1;
            
        }
        else if(inUseArray[cpu->LSQ.rs1] == 1 && inUseArray[cpu->LSQ.rs2] == 1) 
        {
            
                //printf("\n\nHi\n\n");
                cpu->LSQ.rs1_value = resultBuffer[cpu->LSQ.rs1];
                cpu->LSQ.rs2_value = resultBuffer[cpu->LSQ.rs2];

                //resultBuffer[cpu->LSQ.rs1] = -1;
                //resultBuffer[cpu->LSQ.rs2] = -1;
            
        }
        else if(inUseArray[cpu->LSQ.rs2] == 1 && inUseArray[cpu->LSQ.rs3] == 1) 
        {
            
                cpu->LSQ.rs2_value = resultBuffer[cpu->LSQ.rs2];
                cpu->LSQ.rs3_value = resultBuffer[cpu->LSQ.rs3];
                
                //resultBuffer[cpu->LSQ.rs2] = -1;
                //resultBuffer[cpu->LSQ.rs3] = -1;
            
        }
        else if(inUseArray[cpu->LSQ.rs1] == 1 && inUseArray[cpu->LSQ.rs3] == 1) 
        {
            
                cpu->LSQ.rs1_value = resultBuffer[cpu->LSQ.rs1];
                cpu->LSQ.rs3_value = resultBuffer[cpu->LSQ.rs3];

                //resultBuffer[cpu->LSQ.rs1] = -1;
                //resultBuffer[cpu->LSQ.rs3] = -1;
            
            
        }
        else if(inUseArray[cpu->LSQ.rs1] == 1) 
        {
            
                cpu->LSQ.rs1_value = resultBuffer[cpu->LSQ.rs1];
                //resultBuffer[cpu->LSQ.rs1] = -1;
            

        }
        else if (inUseArray[cpu->LSQ.rs2] == 1)
        {
            
                cpu->LSQ.rs2_value = resultBuffer[cpu->LSQ.rs2];
                //resultBuffer[cpu->LSQ.rs2] = -1;
            
        }
        else if (inUseArray[cpu->LSQ.rs3] == 1)
        {
            
                cpu->LSQ.rs3_value = resultBuffer[cpu->LSQ.rs3];
                //resultBuffer[cpu->LSQ.rs3] = -1;
            
        }
        else
        {
            
        }


        //Forwarding Ends






        
        for(int i=0;i<15;i++)
            {
                if(cpu->LSQ.rd == physicalRegisters[i].dest)
                {
                    physicalRegisters[currentPhysicalRegister].previousPR = physicalRegisters[i].pReg;
                    physicalRegisters[currentPhysicalRegister].takePrevious = TRUE;
                    break;
                }
                else
                {
                    physicalRegisters[currentPhysicalRegister].takePrevious = FALSE;
                }
            }
        

        //printf("\n\n Before : Current Physical Reg = %d\n\n", currentPhysicalRegister);


        if(currentPhysicalRegister == 15)
        currentPhysicalRegister = 0;

        //if(cpu->dispatch.rd != -1 && cpu->dispatch.opcode != OPCODE_CMP)
        // if(cpu->ifu.rd != -1)
        //if(cpu->LSQ.opcode != OPCODE_STORE || cpu->LSQ.opcode != OPCODE_STR || cpu->LSQ.opcode != OPCODE_BZ || cpu->LSQ.opcode != OPCODE_BNZ || cpu->LSQ.opcode != OPCODE_JUMP || cpu->LSQ.opcode != OPCODE_NOP)
        if(cpu->LSQ.opcode != OPCODE_STORE && cpu->LSQ.opcode != OPCODE_STR && cpu->LSQ.opcode != OPCODE_BZ && cpu->LSQ.opcode != OPCODE_BNZ && cpu->LSQ.opcode != OPCODE_JUMP && cpu->LSQ.opcode != OPCODE_NOP)
        {
            cpu->LSQ.currentPReg = currentPhysicalRegister;
            //physicalRegisters[currentPhysicalRegister] = 1;
            
            physicalRegisters[currentPhysicalRegister].dest = cpu->LSQ.rd;
            physicalRegisters[currentPhysicalRegister].pReg = currentPhysicalRegister;

            currentPhysicalRegister++;
        }

        //printf("\n\n After : Current Physical Reg = %d\n\n", currentPhysicalRegister);

        int rs1ValueLSQ = 0, rs2ValueLSQ = 0, rs3ValueLSQ = 0;
        switch(cpu->LSQ.opcode)
        {

            case OPCODE_LOAD:
            {
                
                for(int i=0;i<15;i++)
                {
                    if(cpu->LSQ.rs1 == physicalRegisters[i].dest)
                    {
                        rs1ValueLSQ = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2ValueLSQ = cpu->LSQ.rs1_value;
                        break;
                    }
                }

                cpu->LSQ.memory_address
                    = rs1ValueLSQ + cpu->LSQ.imm;

                //printf("\n\ncpu->LSQ.memory_address:%d\n\n",cpu->LSQ.memory_address);
                
                cpu->LSQ.result_buffer
                        = cpu->data_memory[cpu->LSQ.memory_address];

                resultBuffer[cpu->LSQ.rd] = cpu->LSQ.result_buffer;

                cpu->phy_regs[cpu->LSQ.currentPReg] = cpu->LSQ.result_buffer;

                

                //printf("\n\ncpu->LSQ.result_buffer:%d\n\n",cpu->LSQ.result_buffer);



                ///Comment

                // cpu->ifu.memory_address
                //     = cpu->ifu.rs1_value + cpu->ifu.imm;

                // cpu->ifu.result_buffer
                //         = cpu->data_memory[cpu->ifu.memory_address];

                // resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                // cpu->phy_regs[cpu->ifu.rd] = cpu->ifu.result_buffer;

                //resultBuffer[cpu->ifu.rd] = -1;

                break;
            }

            case OPCODE_STORE:
            {
                cpu->LSQ.result_buffer = cpu->LSQ.rs1_value;
                cpu->LSQ.memory_address
                    = cpu->LSQ.rs2_value + cpu->LSQ.imm;

                cpu->data_memory[cpu->LSQ.memory_address] = cpu->LSQ.result_buffer;
                resultBuffer[cpu->LSQ.rd] = cpu->LSQ.result_buffer;
                    //printf("\n cpu->execute.rs2_value : %d , cpu->execute.imm %d \n", cpu->execute.rs2_value, cpu->execute.imm);
                break;
            }

            case OPCODE_STR:
            {

                for(int i=0;i<15;i++)
                {
                    if(cpu->LSQ.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1ValueLSQ = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1ValueLSQ = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1ValueLSQ = cpu->LSQ.rs1_value;
                        break;
                    }
                }
                for(int i=0;i<15;i++)
                {
                    if(cpu->LSQ.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2ValueLSQ = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2ValueLSQ = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2ValueLSQ = cpu->LSQ.rs2_value;
                        break;
                    }
                }
                for(int i=0;i<15;i++)
                {
                    if(cpu->LSQ.rs3 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs3ValueLSQ = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs3ValueLSQ = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs3Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs3ValueLSQ = cpu->LSQ.rs3_value;
                        break;
                    }
                }


                cpu->LSQ.result_buffer
                    = rs1ValueLSQ;

                resultBuffer[cpu->LSQ.rd] = cpu->LSQ.result_buffer;

                cpu->phy_regs[cpu->LSQ.currentPReg] = cpu->LSQ.result_buffer;

                cpu->LSQ.memory_address
                     = rs2ValueLSQ + rs3ValueLSQ;

                cpu->data_memory[cpu->LSQ.memory_address] = cpu->LSQ.result_buffer;



                break;
            }

            case OPCODE_LDR:
            {

                for(int i=0;i<15;i++)
                {
                    if(cpu->LSQ.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1ValueLSQ = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1ValueLSQ = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1ValueLSQ = cpu->LSQ.rs1_value;
                        break;
                    }
                }

                for(int i=0;i<15;i++)
                {
                    if(cpu->LSQ.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2ValueLSQ = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2ValueLSQ = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2ValueLSQ = cpu->LSQ.rs2_value;
                        break;
                    }
                }

                cpu->LSQ.memory_address
                    = rs1ValueLSQ + rs2ValueLSQ;

                cpu->LSQ.result_buffer
                    = cpu->data_memory[cpu->LSQ.memory_address];

                //resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                cpu->phy_regs[cpu->LSQ.currentPReg] = cpu->LSQ.result_buffer;

                resultBuffer[cpu->LSQ.rd] = -1;


                break;
            }




        }






        cpu->ROB = cpu->LSQ;
        if(cpu->LSQ.opcode == OPCODE_LOAD || cpu->LSQ.opcode == OPCODE_LDR)
        cpu->mulWB = cpu->LSQ;
        else
        cpu->writeback = cpu->LSQ;
        //cpu->ifu = cpu->LSQ;

        // if (cpu->LSQ.has_insn && cpu->LSQ.isStalled == FALSE)
        // {
        //     if (ENABLE_DEBUG_MESSAGES && cpu->LSQ.opcode != OPCODE_HALT)
        //     {
        //         print_stage_content("LSQ", &cpu->LSQ);
        //     }
        // }
        // else{
        //     if(ENABLE_DEBUG_MESSAGES)
        //     printf("LSQ            : EMPTY\n");
        // }

        if (ENABLE_DEBUG_MESSAGES)
        {
            if(cpu->LSQ.has_insn == FALSE || cpu->LSQ.opcode == OPCODE_HALT)
            printf("LSQ            : EMPTY\n");
            else
            {
                print_stage_content("LSQ", &cpu->LSQ);
                cpu->LSQ.has_insn = FALSE;
            }
        }
        else
        printf("LSQ            : EMPTY\n");
    }
    else
    {
        if(ENABLE_DEBUG_MESSAGES)
        {
            if(cpu->LSQ.has_insn == FALSE || cpu->LSQ.opcode == OPCODE_HALT)
            {
                printf("LSQ            : EMPTY\n");
            }
            else
            print_stage_content("LSQ", &cpu->LSQ);
        }
    }
}

static void
APEX_ROB(APEX_CPU *cpu)
{
    if(cpu->ROB.has_insn)
    {



        // ROB[currentIndex] = cpu->ROB;
        // currentIndex++;

        //for(int i=0;i<)




        // if (cpu->ROB.has_insn && cpu->ROB.isStalled == FALSE)
        // {
        //     if (ENABLE_DEBUG_MESSAGES && cpu->ROB.opcode != OPCODE_HALT)
        //     {
        //         print_stage_content("ROB", &cpu->ROB);
        //     }
        // }
        // else{
        //     if(ENABLE_DEBUG_MESSAGES)
        //     printf("ROB            : EMPTY\n");
        // }

        // APEX_Instruction currentInsROB;

        // strcpy(currentInsROB->opcode_str, cpu->ROB.opcode_str);
        // currentInsROB->opcode = cpu->ROB.opcode;
        // currentInsROB->rd = cpu->ROB.rd;
        // currentInsROB->rs1 = cpu->ROB.rs1;
        // currentInsROB->rs2 = cpu->ROB.rs2;
        // currentInsROB->rs3 = cpu->ROB.rs3;
        // currentInsROB->imm = cpu->ROB.imm;

        // ROB[0] = currentInsROB;

        // if(currentIndex == 3)
        // currentIndex = 0;

        // ROB[currentIndex] = cpu->ROB;
        // currentIndex++;

        // printf("\n\nROB[0] Q0 : %s %d %d %d\n\n", ROB[0].opcode_str,ROB[0].rd,ROB[0].rs1,ROB[0].rs2);
        // printf("\n\nROB[1] Q1 : %s %d %d %d\n\n", ROB[1].opcode_str,ROB[1].rd,ROB[1].rs1,ROB[1].rs2);
        // printf("\n\nROB[2] Q2 : %s %d %d %d\n\n", ROB[2].opcode_str,ROB[2].rd,ROB[2].rs1,ROB[2].rs2);

        // if(ROB[2].opcode != '\0')
        // {
        //     if(sendingIndex == 3)
        //     sendingIndex = 0;

        //     cpu->writeback = ROB[sendingIndex];
        //     sendingIndex++;

        // }

        // if (ROB[0].opcode != '\0')
        // {
        //     if(sendingIndex == 3)
        //     sendingIndex = 0;

        //     //printf("\n\n\nSENTTTT\n\n\n");
        //     if(sendingIndex == 0)
        //     cpu->writeback = ROB[0];

        //     sendingIndex++;



        //     // if(ROB[0].opcode == OPCODE_MOVC)
        //     // cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;

        // }
        // else
        // {
        //     //nothing
        // }

        

        // if (ROB[3].opcode != '\0')
        // {
        //     printf("\n\n\nSENTTTT\n\n\n");
        //     cpu->writeback = cpu->ROB;
        // }
        // else
        // {
        //     //nothing
        // }

        // if (ROB[3].opcode == '\0')
        // printf("\n\nNULL\n\n");
        // else
        // printf("\n\nNOT NULL\n\n");



        



        //printf("\n\nROB[3].rd : %s\n\n", ROB[3].opcode_str);
        //Nothing
        //print_stage_content("ROB", &currentInsROB);


        if (ENABLE_DEBUG_MESSAGES)
            {
                // if(cpu->ROB.has_insn == FALSE || cpu->ROB.opcode == OPCODE_HALT)
                // printf("\nROB            : EMPTY\n");
                if(cpu->ROB.opcode == OPCODE_HALT)
                print_stage_content("ROB", &cpu->ROB);
                else
                {
                    print_stage_content("ROB", &cpu->ROB);
                    //printf("ROB            : EMPTY\n");
                    cpu->ROB.has_insn = FALSE;
                }
                // else
                // {
                //     printf("\n");
                //     print_stage_content("ROB", &cpu->ROB);
                //     cpu->ROB.has_insn = FALSE;
                // }
            }
            else
            printf("ROB              : EMPTY\n");
    }
    else if(cpu->ROB.opcode == OPCODE_HALT)
    {
        printf("ROB              : EMPTY\n\n");
        //cpu->decode = cpu->fetch;
    }
    else
    {
        if(ENABLE_DEBUG_MESSAGES)
        {
            if(cpu->ROB.has_insn == FALSE || cpu->ROB.opcode == OPCODE_HALT)
            {
                printf("ROB            : EMPTY\n");
            }
            else
            print_stage_content("ROB", &cpu->ROB);
        }
    }

    
}



static void
APEX_ifu(APEX_CPU *cpu)
{
    //printf("\n\nIfu working\n\n");
    if (cpu->ifu.has_insn)
    {


        //printf("\n\nCurrent Physical Reg = %d\n\n", currentPhysicalRegister);


        if(currentPhysicalRegister == 15)
        currentPhysicalRegister = 0;

        //if(cpu->dispatch.rd != -1 && cpu->dispatch.opcode != OPCODE_CMP)
        // if(cpu->ifu.rd != -1)
        if(cpu->ifu.opcode != OPCODE_STORE && cpu->ifu.opcode != OPCODE_STR && cpu->ifu.opcode != OPCODE_BZ && cpu->ifu.opcode != OPCODE_BNZ && cpu->ifu.opcode != OPCODE_JUMP && cpu->ifu.opcode != OPCODE_NOP)
        {
            for(int i=0;i<15;i++)
            {
                if(cpu->ifu.rd == physicalRegisters[i].dest)
                {
                    physicalRegisters[currentPhysicalRegister].previousPR = physicalRegisters[i].pReg;
                    physicalRegisters[currentPhysicalRegister].takePrevious = TRUE;
                    break;
                }
                else
                {
                    physicalRegisters[currentPhysicalRegister].takePrevious = FALSE;
                }
            }

            //printf("\n\npreviousPhysicalRegister = %d, currentPhysicalregister = %d\n\n", physicalRegisters[currentPhysicalRegister].previousPR, currentPhysicalRegister);

            cpu->ifu.currentPReg = currentPhysicalRegister;
            //physicalRegisters[currentPhysicalRegister] = 1;
            
            physicalRegisters[currentPhysicalRegister].dest = cpu->ifu.rd;
            physicalRegisters[currentPhysicalRegister].pReg = currentPhysicalRegister;

            

            currentPhysicalRegister++;
        }




        // if(ifuCount == 0)
        // ifuStalled=0;

        //branchTaken = 0;

        int rs1Value = 0, rs2Value = 0;

        /* Execute logic based on instruction type */
        switch (cpu->ifu.opcode)
        {
            case OPCODE_ADD:
            {
                // if(physicalRegisters[cpu->ifu.currentPReg] != -1 && physicalRegisters[cpu->ifu.rs2] != -1)
                // {

                // }

                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1Value = cpu->ifu.rs1_value;
                        break;
                    }
                }

                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2Value = cpu->ifu.rs2_value;
                        break;
                    }
                }

                


                // cpu->ifu.result_buffer
                //     = cpu->ifu.rs1_value + cpu->ifu.rs2_value;

                cpu->ifu.result_buffer = rs1Value + rs2Value;

                resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                cpu->phy_regs[cpu->ifu.currentPReg] = cpu->ifu.result_buffer;



                /* Set the zero flag based on the result buffer */
                if (cpu->ifu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_ADDL:
            {

                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        
                        break;
                    }
                    else
                    {
                        rs1Value = cpu->ifu.rs1_value;
                        break;
                    }
                }

                cpu->ifu.result_buffer = rs1Value + cpu->ifu.imm;

                resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                cpu->phy_regs[cpu->ifu.currentPReg] = cpu->ifu.result_buffer;

                // cpu->ifu.result_buffer
                //     = cpu->ifu.rs1_value + cpu->ifu.imm;


                //printf("\n\ncpu->ifu.result_buffer : %d\n\n", cpu->ifu.result_buffer);
                // resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                // cpu->phy_regs[cpu->ifu.rd] = cpu->ifu.result_buffer;
                //printf("\n\nresultBuffer[cpu->ifu.rd] : %d\n\n", resultBuffer[cpu->ifu.rd]);
                /* Set the zero flag based on the result buffer */
                if (cpu->ifu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUB:
            {

                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1Value = cpu->ifu.rs1_value;
                        break;
                    }
                }

                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2Value = cpu->ifu.rs2_value;
                        break;
                    }
                }

                cpu->ifu.result_buffer = rs1Value - rs2Value;

                resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                cpu->phy_regs[cpu->ifu.currentPReg] = cpu->ifu.result_buffer;



                // cpu->ifu.result_buffer
                //     = cpu->ifu.rs1_value - cpu->ifu.rs2_value;

                // resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                // cpu->phy_regs[cpu->ifu.rd] = cpu->ifu.result_buffer;

                /* Set the zero flag based on the result buffer */
                if (cpu->ifu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUBL:
            {

                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1Value = cpu->ifu.rs1_value;
                        break;
                    }
                }

                cpu->ifu.result_buffer = rs1Value - cpu->ifu.imm;

                resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                cpu->phy_regs[cpu->ifu.currentPReg] = cpu->ifu.result_buffer;


                // cpu->ifu.result_buffer
                //     = cpu->ifu.rs1_value - cpu->ifu.imm;

                // resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                // cpu->phy_regs[cpu->ifu.rd] = cpu->ifu.result_buffer;

                /* Set the zero flag based on the result buffer */
                if (cpu->ifu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_DIV:
            {

                



                if(cpu->ifu.rs2_value == 0)
                {
                    cpu->ifu.result_buffer = 0;
                }
                else
                {

                    for(int i=0;i<15;i++)
                    {
                        if(cpu->ifu.rs1 == physicalRegisters[i].dest)
                        {
                            if(physicalRegisters[i].takePrevious == TRUE)
                            rs1Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                            else
                            rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                            //rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                            break;
                        }
                        else
                        {
                            rs1Value = cpu->ifu.rs1_value;
                            break;
                        }
                    }

                    for(int i=0;i<15;i++)
                    {
                        if(cpu->ifu.rs2 == physicalRegisters[i].dest)
                        {
                            if(physicalRegisters[i].takePrevious == TRUE)
                            rs2Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                            else
                            rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                            //rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                            break;
                        }
                        else
                        {
                            rs2Value = cpu->ifu.rs2_value;
                            break;
                        }
                    }

                    cpu->ifu.result_buffer = rs1Value / rs2Value;

                    resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                    cpu->phy_regs[cpu->ifu.currentPReg] = cpu->ifu.result_buffer;


                    // cpu->ifu.result_buffer
                    // = cpu->ifu.rs1_value / cpu->ifu.rs2_value;

                    // resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                    // cpu->phy_regs[cpu->ifu.rd] = cpu->ifu.result_buffer;
                }
                
                /* Set the zero flag based on the result buffer */
                if (cpu->ifu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_BZ:
            {

                // if (cpu->zero_flag == TRUE)
                if (tempZflag == TRUE)
                {
                    //printf("\n\nBZ working\n\n");
                    //cpu->ifu.bTaken = TRUE;

                    takenBranches[cpu->ifu.pc] = cpu->ifu.pc;
                    //currentTakenBranch++;



                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->ifu.pc + cpu->ifu.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                    * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;
                    cpu->IQ.has_insn = FALSE;
                    cpu->LSQ.has_insn = FALSE;
                    cpu->dispatch.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                    cpu->fetch.isStalled = FALSE;
                    cpu->decode.isStalled = FALSE;
                    
                }
                break;
                    
            }

            case OPCODE_JUMP:
            {

                //branchTaken = 0;
                // if (cpu->zero_flag == TRUE)
                // {
                    //printf("\n\nHI\n\n");
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->ifu.rs1_value + cpu->ifu.imm;

                    //printf("\n\ncpu->pc value : %d\n\n", cpu->pc);
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                    * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;


                    // cpu->fetch.isStalled = FALSE;
                    // cpu->decode.isStalled = FALSE;
                    
                //}
                break;

            }

            case OPCODE_BNZ:
            {
                // if(cpu->ifu.bTaken == TRUE)
                // cpu->zero_flag = TRUE;


                // if(cpu->ifu.bTaken == TRUE)
                // {
                //     //printf("\n\nHIiiiii\n\n");
                //     cpu->pc = cpu->ifu.pc+4;
                //     cpu->fetch_from_next_cycle = TRUE;

                //     cpu->decode.has_insn = FALSE;
                //     cpu->dispatch.has_insn = FALSE;
                //     cpu->IQ.has_insn = FALSE;
                //     cpu->ROB.has_insn = FALSE;

                //     cpu->fetch.has_insn = TRUE;
                    
                //     break;
                // }
                // else if(cpu->zero_flag == FALSE)
                // {
                //     //cpu->ifu.bTaken = TRUE;

                //     takenBranches[cpu->ifu.pc] = cpu->ifu.pc;
                //     //currentTakenBranch++;
                //     //printf("\n\ncpu->zero_flag = %d\n\n",cpu->zero_flag);
                //     //printf("\n\ncpu->ifu.bTaken : %d\n\n", cpu->ifu.bTaken);

                //     if(cpu->ifu.bTaken == FALSE)
                //     {
                //         //printf("\n\ncpu->ifu.bTaken : %d\n\n", cpu->ifu.bTaken);
                //         /* Calculate new PC, and send it to fetch unit */
                //         cpu->pc = cpu->ifu.pc + cpu->ifu.imm;
                    
                //         /* Since we are using reverse callbacks for pipeline stages, 
                //         * this will prevent the new instruction from being fetched in the current cycle*/
                //         cpu->fetch_from_next_cycle = TRUE;

                //         /* Flush previous stages */
                //         cpu->decode.has_insn = FALSE;
                //         cpu->dispatch.has_insn = FALSE;
                //         cpu->IQ.has_insn = FALSE;
                //         cpu->ROB.has_insn = FALSE;

                //         /* Make sure fetch stage is enabled to start fetching from new PC */
                //         cpu->fetch.has_insn = TRUE;
                //     }
                //     else
                //     {
                //         //printf("\n\ncpu->ifu.pc:%d\n\n", cpu->ifu.pc);
                //         cpu->pc = cpu->ifu.pc+4;
                //         cpu->fetch_from_next_cycle = TRUE;

                //         cpu->decode.has_insn = FALSE;
                //         cpu->dispatch.has_insn = FALSE;
                //         cpu->IQ.has_insn = FALSE;
                //         cpu->ROB.has_insn = FALSE;

                //         cpu->fetch.has_insn = TRUE;
                        
                //         break;
                //     }
                    
                // }

                // printf("\n\ncpu->ifu.bTaken = %d, tempZflag = %d\n\n", cpu->ifu.bTaken, tempZflag);

                //if(cpu->ifu.bTaken == TRUE && cpu->zero_flag == TRUE)
                if(cpu->ifu.imm <0)
                {
                    if(cpu->ifu.bTaken == TRUE && tempZflag == FALSE)
                    {
                        takenBranches[cpu->ifu.pc] = cpu->ifu.pc;
                        //printf("\n\nHIiiiii\n\n");
                        //cpu->pc = cpu->ifu.pc+4;
                        //cpu->fetch_from_next_cycle = TRUE;

                        // cpu->decode.has_insn = FALSE;
                        // cpu->dispatch.has_insn = FALSE;
                        // cpu->IQ.has_insn = FALSE;
                        // cpu->ROB.has_insn = FALSE;

                        //cpu->fetch.has_insn = TRUE;
                        
                        break;
                    }
                    else if(cpu->ifu.bTaken == TRUE && tempZflag == TRUE)
                    {
                        cpu->pc = cpu->ifu.pc+4;
                        cpu->fetch_from_next_cycle = TRUE;

                        cpu->decode.has_insn = FALSE;
                        cpu->dispatch.has_insn = FALSE;
                        cpu->IQ.has_insn = FALSE;
                        cpu->ROB.has_insn = FALSE;

                        cpu->fetch.has_insn = TRUE;
                        
                        break;
                    }
                    else if(cpu->ifu.bTaken == FALSE && tempZflag == FALSE)
                    {
                        //printf("\n\nBNZ working\n\n");
                        //cpu->ifu.bTaken = TRUE;

                        takenBranches[cpu->ifu.pc] = cpu->ifu.pc;
                        //currentTakenBranch++;
                        //printf("\n\ncpu->zero_flag = %d\n\n",cpu->zero_flag);
                        //printf("\n\ncpu->ifu.bTaken : %d\n\n", cpu->ifu.bTaken);

                        // if(cpu->ifu.bTaken == FALSE)
                        // {
                            //printf("\n\ncpu->ifu.bTaken : %d\n\n", cpu->ifu.bTaken);
                            /* Calculate new PC, and send it to fetch unit */
                            cpu->pc = cpu->ifu.pc + cpu->ifu.imm;
                        
                            /* Since we are using reverse callbacks for pipeline stages, 
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            /* Flush previous stages */
                            cpu->decode.has_insn = FALSE;
                            cpu->dispatch.has_insn = FALSE;
                            cpu->IQ.has_insn = FALSE;
                            cpu->ROB.has_insn = FALSE;

                            /* Make sure fetch stage is enabled to start fetching from new PC */
                            cpu->fetch.has_insn = TRUE;
                            break;
                        //}
                        // else
                        // {
                        //     //printf("\n\ncpu->ifu.pc:%d\n\n", cpu->ifu.pc);
                        //     cpu->pc = cpu->ifu.pc+4;
                        //     cpu->fetch_from_next_cycle = TRUE;

                        //     cpu->decode.has_insn = FALSE;
                        //     cpu->dispatch.has_insn = FALSE;
                        //     cpu->IQ.has_insn = FALSE;
                        //     cpu->ROB.has_insn = FALSE;

                        //     cpu->fetch.has_insn = TRUE;
                            
                        //     break;
                        // }
                        
                    }                
                    else if(cpu->ifu.bTaken == FALSE && tempZflag == TRUE)
                    {

                        break;
                    }
                }
                else
                {
                    if(tempZflag == FALSE)
                    {
                        /* Calculate new PC, and send it to fetch unit */
                   
                    cpu->pc = cpu->ifu.pc + cpu->ifu.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    //cpu->decode.has_insn = FALSE;
                    cpu->decode.has_insn = FALSE;
                    cpu->dispatch.has_insn = FALSE;
                    cpu->IQ.has_insn = FALSE;
                    cpu->ROB.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;

                    }
                }
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->ifu.result_buffer = cpu->ifu.imm;

                resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                cpu->phy_regs[cpu->ifu.currentPReg] = cpu->ifu.result_buffer;

                /* Set the zero flag based on the result buffer */
                if (cpu->ifu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_HALT:
            {
                /* HALT doesn't have register operands */
                break;
            }
 

            case OPCODE_CMP:
            {
                //printf("\n\nresultBuffer[cpu->ifu.rd] : %d\n\n", resultBuffer[cpu->ifu.rd]);
                

                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1Value = cpu->ifu.rs1_value;
                        break;
                    }
                }
                for(int i=0;i<15;i++)
                {
                    if(cpu->ifu.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2Value = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2Value = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2Value = cpu->ifu.rs2_value;
                        break;
                    }
                }

                //printf("\n\ncpu->ifu.rs1_value = %d; cpu->ifu.rs2_value = %d\n\n", rs1Value, rs2Value);

                cpu->ifu.result_buffer
                    = rs1Value - rs2Value;

                

                //resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                //cpu->phy_regs[cpu->ifu.currentPReg] = cpu->ifu.result_buffer;

                // cpu->ifu.result_buffer
                //     = cpu->ifu.rs1_value - cpu->ifu.rs2_value;

                // resultBuffer[cpu->ifu.rd] = cpu->ifu.result_buffer;

                // cpu->phy_regs[cpu->ifu.rd] = cpu->ifu.result_buffer;

                //printf("\n\ncpu->ifu.result_buffer = %d\n\n", cpu->ifu.result_buffer);

                



                if(rs1Value == rs2Value)
                tempZflag = TRUE;
                else
                tempZflag = FALSE;

                //printf("\n\ntempZFlag = %d\n\n", tempZflag);

                // /* Set the zero flag based on the result buffer */
                // if (cpu->ifu.result_buffer != 0)
                // {
                //     cpu->zero_flag = FALSE;
                //     tempZflag = FALSE;
                //     //printf("\n\nTaken Z flag as False\n\n");

                //     //cpu->phy_regs[cpu->ifu.currentPReg] = 0;
                //     //resultBuffer[cpu->ifu.rd] = 0;

                // }
                // else
                // {
                //     cpu->zero_flag = TRUE;
                //     tempZflag = TRUE;
                //     //cpu->phy_regs[cpu->ifu.currentPReg] = 1;
                //     //resultBuffer[cpu->ifu.rd] = 1;
                // }


                //printf("\n\nAfter : cpu->ifu.result_buffer = %d\n\n", cpu->ifu.result_buffer);

                break;
            }

            case OPCODE_NOP:
            {
                /* NOP doesn't have register operands */
                break;
            }

            default:
                break;

        }

        // if(currentIndex == 3)
        // currentIndex = 0;

        // ROB[currentIndex] = cpu->ifu;
        // currentIndex++;

        cpu->writeback = cpu->ifu;


        // if (ROB[3].opcode != '\0')
        // {
        //     printf("\n\n\nSENTTTT\n\n\n");
        //     cpu->writeback = ROB[3];
        // }
        // else
        // {
        //     //nothing
        // }

        if (ENABLE_DEBUG_MESSAGES && cpu->ifu.opcode != OPCODE_HALT)
        {
            print_stage_content("IFU", &cpu->ifu);
            cpu->ifu.has_insn = FALSE;
        }
    }
    else{
        if(ENABLE_DEBUG_MESSAGES)
        printf("IFU            : EMPTY\n");
    }
}

static void
APEX_mul4(APEX_CPU *cpu)
{
    int rs1ValueMul = 0, rs2ValueMul = 0;
    if (cpu->mul4.has_insn)
    {

        /* Execute logic based on instruction type */
        switch (cpu->mul4.opcode)
        {

            

            case OPCODE_MUL:
            {
                ifuCount=1;

                for(int i=0;i<15;i++)
                {
                    if(cpu->mul4.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1ValueMul = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1ValueMul = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1ValueMul = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1ValueMul = cpu->mul4.rs1_value;
                        break;
                    }
                }

                for(int i=0;i<15;i++)
                {
                    if(cpu->mul4.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2ValueMul = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2ValueMul = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2ValueMul = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2ValueMul = cpu->mul4.rs2_value;
                        break;
                    }
                }

                


                // cpu->ifu.result_buffer
                //     = cpu->ifu.rs1_value + cpu->ifu.rs2_value;

                //printf("\n\nrs1Value : %d; rs2Value : %d\n\n", rs1ValueMul, rs2ValueMul);

                cpu->mul4.result_buffer = rs1ValueMul * rs2ValueMul;

                resultBuffer[cpu->mul4.rd] = cpu->mul4.result_buffer;

                //printf("\n\ncurrentPreg : %d;\n\n", cpu->mul4.currentPReg);

                cpu->phy_regs[cpu->mul4.currentPReg] = cpu->mul4.result_buffer;

                cpu->regs[cpu->mul4.rd] = cpu->mul4.result_buffer;

                // cpu->mul4.result_buffer
                //     = cpu->mul4.rs1_value * cpu->mul4.rs2_value;

                // resultBuffer[cpu->mul4.rd] = cpu->mul4.result_buffer;

                // cpu->phy_regs[cpu->mul4.rd] = cpu->mul4.result_buffer;

                /* Set the zero flag based on the result buffer */
                if (cpu->mul4.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            default:
                break;

        }

        // if(currentIndex == 3)
        // currentIndex = 0;

        // ROB[currentIndex] = cpu->mul4;
        // currentIndex++;
        
        /* Copy data from execute latch to memory latch*/
        //cpu->mulWB = cpu->mul4;
        cpu->mul4.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->mul4.opcode != OPCODE_HALT)
        {
            print_stage_content("MUL4", &cpu->mul4);
        }
    }
    else{
        if(ENABLE_DEBUG_MESSAGES)
        printf("MUL4           : EMPTY\n");
    }
}

static void
APEX_mulWB(APEX_CPU *cpu)
{
    if(cpu->mulWB.has_insn)
    {
        switch(cpu->mulWB.opcode)
    {
        case OPCODE_LDR:
            {
                cpu->regs[cpu->mulWB.rd] = cpu->mulWB.result_buffer;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->mulWB.rd] = cpu->mulWB.result_buffer;

                //resultBuffer[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }
    }
    }
}

static void
APEX_mul2(APEX_CPU *cpu)
{
    if (cpu->mul2.has_insn)
    {

        
        
        /* Copy data from execute latch to memory latch*/
        cpu->mul3 = cpu->mul2;
        cpu->mul2.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->mul2.opcode != OPCODE_HALT)
        {
            print_stage_content("MUL2", &cpu->mul2);
        }
    }
    else{
        if(ENABLE_DEBUG_MESSAGES)
        printf("MUL2           : EMPTY\n");
    }
}

static void
APEX_mul3(APEX_CPU *cpu)
{
    if (cpu->mul3.has_insn)
    {

        

        
        
        /* Copy data from execute latch to memory latch*/
        cpu->mul4 = cpu->mul3;
        cpu->mul3.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->mul3.opcode != OPCODE_HALT)
        {
            print_stage_content("MUL3", &cpu->mul3);
        }
    }
    else{
        if(ENABLE_DEBUG_MESSAGES)
        printf("MUL3           : EMPTY\n");
    }
}

static void
APEX_mul1(APEX_CPU *cpu)
{
    if (cpu->mul1.has_insn)
    {

        for(int i=0;i<15;i++)
            {
                if(cpu->mul1.rd == physicalRegisters[i].dest)
                {
                    physicalRegisters[currentPhysicalRegister].previousPR = physicalRegisters[i].pReg;
                    physicalRegisters[currentPhysicalRegister].takePrevious = TRUE;
                    break;
                }
                else
                {
                    physicalRegisters[currentPhysicalRegister].takePrevious = FALSE;
                }
            }


        //printf("\n\nCurrent Physical Reg = %d\n\n", currentPhysicalRegister);


            if(currentPhysicalRegister == 15)
            currentPhysicalRegister = 0;

            //if(cpu->dispatch.rd != -1 && cpu->dispatch.opcode != OPCODE_CMP)
            // if(cpu->ifu.rd != -1)
            // if(cpu->mul1.opcode != OPCODE_STORE || cpu->mul1.opcode != OPCODE_STR || cpu->mul1.opcode != OPCODE_BZ || cpu->mul1.opcode != OPCODE_BNZ || cpu->mul1.opcode != OPCODE_JUMP || cpu->mul1.opcode != OPCODE_NOP)
            // {
                cpu->mul1.currentPReg = currentPhysicalRegister;
                //physicalRegisters[currentPhysicalRegister] = 1;
                
                physicalRegisters[currentPhysicalRegister].dest = cpu->mul1.rd;
                physicalRegisters[currentPhysicalRegister].pReg = currentPhysicalRegister;

                currentPhysicalRegister++;
            //}

        /* Copy data from execute latch to memory latch*/
        cpu->mul2 = cpu->mul1;
        cpu->mul1.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->mul1.opcode != OPCODE_HALT)
        {
            print_stage_content("MUL1", &cpu->mul1);
        }
    }
    else{
        if(ENABLE_DEBUG_MESSAGES)
        printf("MUL1           : EMPTY\n");
    }
}

static void
APEX_lsfu(APEX_CPU *cpu)
{
    int rs1ValueLsfu = 0, rs2ValueLsfu = 0;

    if (cpu->lsfu.has_insn && cpu->lsfu.isStalled == FALSE)
    {

        for(int i=0;i<15;i++)
        {
            if(cpu->lsfu.rd == physicalRegisters[i].dest)
            {
                physicalRegisters[currentPhysicalRegister].previousPR = physicalRegisters[i].pReg;
                physicalRegisters[currentPhysicalRegister].takePrevious = TRUE;
                break;
            }
            else
            {
                physicalRegisters[currentPhysicalRegister].takePrevious = FALSE;
            }
        }

        //printf("\n\nCurrent Physical Reg = %d\n\n", currentPhysicalRegister);


        if(currentPhysicalRegister == 15)
        currentPhysicalRegister = 0;

        //if(cpu->dispatch.rd != -1 && cpu->dispatch.opcode != OPCODE_CMP)
        // if(cpu->ifu.rd != -1)
        //if(cpu->lsfu.opcode != OPCODE_STORE || cpu->lsfu.opcode != OPCODE_STR || cpu->lsfu.opcode != OPCODE_BZ || cpu->lsfu.opcode != OPCODE_BNZ || cpu->lsfu.opcode != OPCODE_JUMP || cpu->lsfu.opcode != OPCODE_NOP)
        if(cpu->lsfu.opcode != OPCODE_STORE && cpu->lsfu.opcode != OPCODE_STR && cpu->lsfu.opcode != OPCODE_BZ && cpu->lsfu.opcode != OPCODE_BNZ && cpu->lsfu.opcode != OPCODE_JUMP && cpu->lsfu.opcode != OPCODE_NOP)
        {
            cpu->lsfu.currentPReg = currentPhysicalRegister;
            //physicalRegisters[currentPhysicalRegister] = 1;
            
            physicalRegisters[currentPhysicalRegister].dest = cpu->lsfu.rd;
            physicalRegisters[currentPhysicalRegister].pReg = currentPhysicalRegister;

            currentPhysicalRegister++;
        }

        
        switch (cpu->lsfu.opcode)
        {

            case OPCODE_AND:
            {

                for(int i=0;i<15;i++)
                {
                    if(cpu->lsfu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1ValueLsfu = cpu->lsfu.rs1_value;
                        break;
                    }
                }

                for(int i=0;i<15;i++)
                {
                    if(cpu->lsfu.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2ValueLsfu = cpu->lsfu.rs2_value;
                        break;
                    }
                }

                //printf("\n\n rs1ValueLsfu = %d; rs2ValueLsfu = %d\n\n", rs1ValueLsfu, rs2ValueLsfu);

                cpu->lsfu.result_buffer
                    = rs1ValueLsfu & rs2ValueLsfu;

                resultBuffer[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                cpu->phy_regs[cpu->lsfu.currentPReg] = cpu->lsfu.result_buffer;

                //printf("\n\nResult buffer = %d\n\n", cpu->lsfu.result_buffer);

                //comment
                // cpu->lsfu.result_buffer
                //     = cpu->lsfu.rs1_value & cpu->lsfu.rs2_value;

                // resultBuffer[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                // cpu->phy_regs[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                /* Set the zero flag based on the result buffer */
                if (cpu->lsfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_OR:
            {

                for(int i=0;i<15;i++)
                {
                    if(cpu->lsfu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1ValueLsfu = cpu->lsfu.rs1_value;
                        break;
                    }
                }

                for(int i=0;i<15;i++)
                {
                    if(cpu->lsfu.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2ValueLsfu = cpu->lsfu.rs2_value;
                        break;
                    }
                }

                cpu->lsfu.result_buffer
                    = rs1ValueLsfu | rs2ValueLsfu;

                resultBuffer[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                cpu->phy_regs[cpu->lsfu.currentPReg] = cpu->lsfu.result_buffer;

                //comment
                // cpu->lsfu.result_buffer
                //     = cpu->lsfu.rs1_value | cpu->lsfu.rs2_value;

                // resultBuffer[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                // cpu->phy_regs[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                /* Set the zero flag based on the result buffer */
                if (cpu->lsfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_XOR:
            {
                for(int i=0;i<15;i++)
                {
                    if(cpu->lsfu.rs1 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs1ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs1ValueLsfu = cpu->lsfu.rs1_value;
                        break;
                    }
                }

                for(int i=0;i<15;i++)
                {
                    if(cpu->lsfu.rs2 == physicalRegisters[i].dest)
                    {
                        if(physicalRegisters[i].takePrevious == TRUE)
                        rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].previousPR];
                        else
                        rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        //rs2ValueLsfu = cpu->phy_regs[physicalRegisters[i].pReg];
                        break;
                    }
                    else
                    {
                        rs2ValueLsfu = cpu->lsfu.rs2_value;
                        break;
                    }
                }

                cpu->lsfu.result_buffer
                    = rs1ValueLsfu ^ rs2ValueLsfu;

                resultBuffer[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                cpu->phy_regs[cpu->lsfu.currentPReg] = cpu->lsfu.result_buffer;

                //commented
                // cpu->lsfu.result_buffer
                //     = cpu->lsfu.rs1_value ^ cpu->lsfu.rs2_value;

                // resultBuffer[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                // cpu->phy_regs[cpu->lsfu.rd] = cpu->lsfu.result_buffer;

                /* Set the zero flag based on the result buffer */
                if (cpu->lsfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            default:
                break;

        }

        // if(currentIndex == 3)
        // currentIndex = 0;

        // ROB[currentIndex] = cpu->lsfu;
        // currentIndex++;

        /* Copy data from execute latch to memory latch*/
        cpu->writeback = cpu->lsfu;
        cpu->lsfu.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->lsfu.opcode != OPCODE_HALT)
        {
            print_stage_content("LOGICAL FU", &cpu->lsfu);
        }
    }
    else{
        if(ENABLE_DEBUG_MESSAGES)
        printf("LOGICAL FU     : EMPTY\n");
    }
}


// /*
//  * Memory Stage of APEX Pipeline
//  *
//  * Note: You are free to edit this function according to your implementation
//  */
// static void
// APEX_memory(APEX_CPU *cpu)
// {
//     if (cpu->memory.has_insn)
//     {
//         switch (cpu->memory.opcode)
//         {
//             case OPCODE_ADD:
//             {
//                 /* No work for ADD */

//                 resultBuffer[cpu->memory.rd] = cpu->memory.result_buffer;
//                 break;
//             }

//             case OPCODE_ADDL:
//             {
//                 /* No work for ADD */

//                 resultBuffer[cpu->execute.rd] = cpu->execute.result_buffer;
//                 break;
//             }

//             case OPCODE_SUB:
//             {
//                 /* No work for SUB */

//                 resultBuffer[cpu->execute.rd] = cpu->execute.result_buffer;
//                 break;
//             }

//             case OPCODE_SUBL:
//             {
//                 /* No work for ADD */

//                 resultBuffer[cpu->execute.rd] = cpu->execute.result_buffer;
//                 break;
//             }

//             case OPCODE_MUL:
//             {
//                 /* No work for MUL */

//                 resultBuffer[cpu->execute.rd] = cpu->execute.result_buffer;
//                 break;
//             }

//             case OPCODE_DIV:
//             {
//                 /* No work for MUL */

//                 resultBuffer[cpu->execute.rd] = cpu->execute.result_buffer;
//                 break;
//             }

//             case OPCODE_LOAD:
//             {
//                 /* Read from data memory */
//                 cpu->memory.result_buffer
//                     = cpu->data_memory[cpu->memory.memory_address];

                
//                 resultBuffer[cpu->memory.rd] = cpu->memory.result_buffer;
//                 //printf("\n\n\ncpu->memory.result_buffer : %d, cpu->memory.rd : %d, resultBuffer[cpu->memory.rd] : %d\n\n\n", cpu->memory.result_buffer, cpu->memory.rd, resultBuffer[cpu->memory.rd]);
//                 //inUseArray[cpu->memory.rd] = 0;
//                 break;
//             }

//             case OPCODE_STORE:
//             {
//                 /* Read from data memory */
//                 cpu->data_memory[cpu->memory.memory_address] = cpu->memory.result_buffer;
//                 //resultBuffer[cpu->memory.rd] = cpu->memory.result_buffer;
//                 //inUseArray[cpu->memory.rd] = 0;
//                 //printf("\n cpu->data_memory[cpu->memory.memory_address] : %d, Address = %d = %d cpu->memory.result_buffer \n", cpu->data_memory[cpu->memory.memory_address], cpu->memory.memory_address, cpu->memory.result_buffer);
//                 break;
//             }

//             case OPCODE_LDR:
//             {
//                 /* Read from data memory */
//                 cpu->memory.result_buffer
//                     = cpu->data_memory[cpu->memory.memory_address];

//                 resultBuffer[cpu->memory.rd] = cpu->memory.result_buffer;
//                 break;
//             }

//             case OPCODE_STR:
//             {
//                 /* Read from data memory */
//                 cpu->data_memory[cpu->memory.memory_address] = cpu->memory.result_buffer;
//                 //resultBuffer[cpu->memory.rd] = cpu->memory.result_buffer;
                
//                 break;
//             }

//             default:
//             {
//                 break;
//             }

//         }

//         if(cpu->memory.opcode == OPCODE_ADD || cpu->memory.opcode == OPCODE_SUB || cpu->memory.opcode == OPCODE_MUL || cpu->memory.opcode == OPCODE_DIV
//             || cpu->memory.opcode == OPCODE_LDR || cpu->memory.opcode == OPCODE_AND || cpu->memory.opcode == OPCODE_OR || cpu->memory.opcode == OPCODE_XOR)
//         {
//             //inUseArray[cpu->memory.rd] = 0;
//             inUseArray[cpu->memory.rd] = 1;
//             // inUseArray[cpu->memory.rs1] = 1;
//             // inUseArray[cpu->memory.rs2] = 1;
//         }
//         else if(cpu->memory.opcode == OPCODE_CMP)
//         {
//             inUseArray[cpu->memory.rd] = 1;
//             //inUseArray[cpu->memory.rs1] = 1;
//         }
//         else if(cpu->memory.opcode == OPCODE_STORE)
//         {
//             inUseArray[cpu->memory.rd] = 0;
//             //inUseArray[cpu->memory.rs2] = 1;
//         }
//         else if(cpu->memory.opcode == OPCODE_ADDL || cpu->memory.opcode == OPCODE_SUBL || cpu->memory.opcode == OPCODE_LOAD)
//         {
//             inUseArray[cpu->memory.rd] = 1;
//             //inUseArray[cpu->memory.rs1] = 1;
//         }
//         else if(cpu->memory.opcode == OPCODE_MOVC)
//         {
//             inUseArray[cpu->memory.rd] = 1;
//         }
//         else if(cpu->memory.opcode == OPCODE_STR)
//         {
//             // inUseArray[cpu->memory.rs1] = 1;
//             // inUseArray[cpu->memory.rs2] = 1;
//             // inUseArray[cpu->memory.rs3] = 1;
//         }
//         else if(cpu->memory.opcode == OPCODE_BNZ || cpu->memory.opcode == OPCODE_BZ)
//         {
//             // inUseArray[cpu->memory.rs1] = 0;
//             // inUseArray[cpu->memory.rs2] = 0;
//             // inUseArray[cpu->memory.rs3] = 0;
//         }
//         else
//         {
//             inUseArray[cpu->memory.rd] = 1;
//         }


        
//         /* Copy data from memory latch to writeback latch*/
//         cpu->writeback = cpu->memory;
//         cpu->memory.has_insn = FALSE;

//         // inUseArray[cpu->memory.rd] = 0;
//         // inUseArray[cpu->memory.rs1] = 0;
//         // inUseArray[cpu->memory.rs2] = 0;
//         // inUseArray[cpu->memory.rs3] = 0;

//         if (ENABLE_DEBUG_MESSAGES && cpu->memory.opcode != OPCODE_HALT)
//         {
//             print_stage_content("Memory", &cpu->memory);
//         }
//         else
//         printf("Memory         : EMPTY\n");

//     }
//     else
//     {
//         if (ENABLE_DEBUG_MESSAGES)
//         {
//             printf("Memory         : EMPTY\n");
//         }   
//     }
// }

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_SUB:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_MUL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_DIV:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;

                //resultBuffer[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_STORE:
            {
                
                //cpu->data_memory[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LDR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_STR:
            {
                //cpu->data_memory[cpu->writeback.memory_address] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_HALT:
            {
                /* HALT doesn't have register operands */
                break;
            }

            case OPCODE_AND:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_OR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_XOR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_BZ:
            {
                break;
            }

            case OPCODE_BNZ:
            {
                break;
            }

            case OPCODE_JUMP:
            {
                break;
            }

            case OPCODE_NOP:
            {
                break;
            }

            case OPCODE_CMP:
            {
                break;
            }

            default:
            {
                break;
            }
        }




        inUseArray[cpu->writeback.rd] = 0;

        





        //printf("\n\nAft : cpu->writeback.rd : %d ; inUseArray[cpu->writeback.rd] : %d\n\n", cpu->writeback.rd, inUseArray[cpu->writeback.rd]);
        //printf("\n\nAft : cpu->execute.rd : %d ; inUseArray[cpu->execute.rd] : %d\n\n", cpu->execute.rd, inUseArray[cpu->execute.rd]);
        
        cpu->lsfu.isStalled = FALSE;
        cpu->mul4.isStalled = FALSE;
        cpu->mul3.isStalled = FALSE;
        cpu->mul2.isStalled = FALSE;
        cpu->mul1.isStalled = FALSE;
        cpu->decode.isStalled = FALSE;
        cpu->fetch.isStalled = FALSE;
        //stop=0;
        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        // if (ENABLE_DEBUG_MESSAGES)
        // {
        //     print_stage_content("Writeback", &cpu->writeback);
        // }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }
    else
    {
        // if (ENABLE_DEBUG_MESSAGES)
        // {
        //     printf("Writeback      : EMPTY\n");
        // }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;
    //cpu->forwarding = forwarding;
    //char forwarding;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    // printf("\n\nEnable Forwarding (Y/N) : \n\n");
    // scanf("%c", &forwarding);
    // printf("\n\n%c\n\n", forwarding);

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    cpu->fetch.isStalled = FALSE;
    cpu->decode.isStalled = FALSE;
    cpu->ifu.isStalled = FALSE;
    cpu->mul4.isStalled = FALSE;
    cpu->mul3.isStalled = FALSE;
    cpu->mul2.isStalled = FALSE;
    cpu->mul1.isStalled = FALSE;
    cpu->lsfu.isStalled = FALSE;
    cpu->execute.isStalled = FALSE;
    cpu->memory.isStalled = FALSE;
    cpu->writeback.isStalled = FALSE;
    cpu->forwarding = FALSE;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->phy_regs, 0, sizeof(int) * PHY_REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}


void print_register_state(APEX_CPU* cpu) 
{
  printf("\n=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n");

  int size = 8;

  for(int i = 0; i < size; ++i)
  {
    printf("|      REG[%d]       |       Value = %d        |          \n", i, cpu->regs[i]);
  }

}

void print_data_memory(APEX_CPU* cpu) {
  printf("\n============== STATE OF DATA MEMORY =============\n");
  for(int i = 0; i < 30; ++i) {
    printf("|        MEM[%d]        |        Data Value = %d        |\n", i, cpu->data_memory[i]);
  }

}

// void print_phy_register_state(APEX_CPU* cpu) 
// {
//   printf("\n=============== STATE OF PHYSICAL REGISTER FILE ==========\n");

//   int size = (int) (sizeof(cpu->phy_regs)/sizeof(cpu->phy_regs[0]));

//   for(int i = 0; i < size; ++i)
//   {
//     printf("|      PHY_REG[%d]       |       Value = %d        |         Status = %s        \n", i, cpu->phy_regs[i], (!inUseArray[i] ? "VALID" : "INVALID"));
//   }

// }

void single_print_data_memory(APEX_CPU* cpu, int n) {
  //printf("\n============== STATE OF DATA MEMORY =============\n");
  //for(int i = 0; i < 30; ++i) {
    printf("\n\n\nSingle requested memory : \n|        MEM[%d]        |        Data Value = %d        |\n\n\n", n, cpu->data_memory[n]);
  //}

}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    //printf("\n\n\nTEST\n\n\n");

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock+1);
            printf("--------------------------------------------\n");
        }
    
        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        // APEX_memory(cpu);
        // APEX_execute(cpu);
        
        APEX_lsfu(cpu);
        APEX_mulWB(cpu);
        APEX_mul4(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_ifu(cpu);
        APEX_ROB(cpu);
        APEX_LSQ(cpu);
        APEX_IQ(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        print_mem_file(cpu);
        print_phy_reg_file(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
    print_register_state(cpu);
    print_data_memory(cpu);
    // print_phy_register_state(cpu);
}

void
APEX_cpu_forward(APEX_CPU *cpu, char ford)
{
    char user_prompt_val;
    //printf("\n\nford = %c\n\n", ford);
    if(ford == 'y' || ford == 'Y')
    cpu->forwarding = TRUE;
    else
    cpu->forwarding = FALSE;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }
    
        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        // APEX_memory(cpu);
        // APEX_execute(cpu);
        APEX_lsfu(cpu);
        APEX_mulWB(cpu);
        APEX_mul4(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_ifu(cpu);
        APEX_ROB(cpu);
        APEX_LSQ(cpu);
        APEX_IQ(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        print_mem_file(cpu);
        print_phy_reg_file(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
            cpu->clock++;
        }

        
    }
    print_register_state(cpu);
    print_data_memory(cpu);
    // print_phy_register_state(cpu);
}

void
APEX_cpu_simulate(APEX_CPU *cpu, int n)
{
    char user_prompt_val;

    while (n != 1)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }
    
        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        // APEX_memory(cpu);
        // APEX_execute(cpu);
        APEX_lsfu(cpu);
        APEX_mulWB(cpu);
        APEX_mul4(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_ifu(cpu);
        APEX_ROB(cpu);
        APEX_LSQ(cpu);
        APEX_IQ(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        print_mem_file(cpu);
        // print_phy_reg_file(cpu);
        

        cpu->clock++;
        n--;
    }
    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }
    
        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        // APEX_memory(cpu);
        // APEX_execute(cpu);
        APEX_lsfu(cpu);
        APEX_mulWB(cpu);
        APEX_mul4(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_ifu(cpu);
        APEX_ROB(cpu);
        APEX_LSQ(cpu);
        APEX_IQ(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        print_mem_file(cpu);
        // print_phy_reg_file(cpu);
        

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
            cpu->clock++;
        }

        
    }
    print_register_state(cpu);
    print_data_memory(cpu);
    // print_phy_register_state(cpu);
}

void
APEX_cpu_display(APEX_CPU *cpu, int n, char ford)
{

    if(ford == 'y' || ford == 'Y')
    cpu->forwarding = TRUE;
    else
    cpu->forwarding = FALSE;

    //int n = 10;
    while (n != 0)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock+1, cpu->insn_completed);
            break;
        }

        // APEX_memory(cpu);
        // APEX_execute(cpu);
        APEX_lsfu(cpu);
        APEX_mulWB(cpu);
        APEX_mul4(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_ifu(cpu);
        APEX_ROB(cpu);
        APEX_LSQ(cpu);
        APEX_IQ(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        // print_phy_reg_file(cpu);

        cpu->clock++;
        n--;
    }

    print_register_state(cpu);
    print_data_memory(cpu);
    // print_phy_register_state(cpu);

}

void
APEX_cpu_show_mem(APEX_CPU *cpu, int new, char ford)
{
    //int n = 10;

    if(ford == 'y' || ford == 'Y')
    cpu->forwarding = TRUE;
    else
    cpu->forwarding = FALSE;


    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock+1, cpu->insn_completed);
            break;
        }

        // APEX_memory(cpu);
        // APEX_execute(cpu);
        APEX_lsfu(cpu);
        APEX_mul4(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_ifu(cpu);
        APEX_ROB(cpu);
        APEX_LSQ(cpu);
        APEX_IQ(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        // print_phy_reg_file(cpu);

        cpu->clock++;
        //n--;
    }
    //print_register_state(cpu);
    //print_data_memory(cpu);
    single_print_data_memory(cpu, new);
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}