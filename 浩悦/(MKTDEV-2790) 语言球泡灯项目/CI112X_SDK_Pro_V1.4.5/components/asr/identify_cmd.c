/**
 * @file identify_cmd.c
 * @brief 易混淆词确认
 * @version 1.0.0
 * @date 2020-4-9
 * 
 * @copyright Copyright (c) 2020  Chipintelli Technology Co., Ltd.
 * 
 */
 
#include <string.h>
#include <stdint.h>
#include "identify_cmd.h"
    
#if (CI_CHIP_TYPE == 1103)
/*此功能仅适用于CI1103芯片
 *如果词A大概率识别成词B，则将词A的易识标记置1，词B的易识标记置0；
 *如果词A与成词B相互误识别，则将词A和词B的易识标记置0；
 *识别混淆词对的属组ID一样，不混淆的词对应的属组ID不一样；
 *{属组ID,命令词,易识标记,易混淆字位置ID},
*/
Confusion_Identify_TypeDef Identify_Struct[]=
{
    //{2,     "制冷模式",   1, 2},
    //{2,     "制热模式",   0, 2},
    {0xFF,}
};

//确认命令词是易混淆的词
int confuse_cmd_is_word(char* cmd)
{
    if(cmd == NULL)
    {
        return NO_CONFUSE_CMD;
    }
    char i = 0;
    while(Identify_Struct[i].group_ID != 0xFF)
    {
        if(!strcmp(cmd,Identify_Struct[i].pCmd))
        {
            return IS_CONFUSE_CMD;
        }
        i++;
    }
    return NO_CONFUSE_CMD;
}

//根据命令词查找属组ID
int confuse_cmd_get_group_id(char* cmd)
{
    int i = 0;
    while(Identify_Struct[i].group_ID != 0xFF)
    {
        if(!strcmp(cmd,Identify_Struct[i].pCmd))
        {
            return Identify_Struct[i].group_ID;
        }
        i++;
    }
    return INVALID_CONFUSE_ID;
}

//根据命令词查找易识别标记
int confuse_cmd_get_legibility_flag(char* cmd)
{
    int i = 0;
    while(Identify_Struct[i].group_ID != 0xFF)
    {
        if(!strcmp(cmd,Identify_Struct[i].pCmd))
        {
            return Identify_Struct[i].legibility_flag;
        }
        i++;
    }
    return INVALID_CONFUSE_ID;
}

//根据命令词查找易混淆字位置
int confuse_cmd_get_identify_word_site(char* cmd)
{
    int i = 0;
    while(Identify_Struct[i].group_ID != 0xFF)
    {
        if(!strcmp(cmd,Identify_Struct[i].pCmd))
        {
            return Identify_Struct[i].identify_word_site;
        }
        i++;
    }
    return INVALID_CONFUSE_ID;
}

//根据命令词查找成对的易混淆词
int confuse_cmd_get_pair_word(char* cmd)
{
    int group_id = 0;
    int i = 0;
    
    group_id = confuse_cmd_get_group_id(cmd);
    while(Identify_Struct[i].group_ID != 0xFF)
    {
        if((Identify_Struct[i].group_ID == group_id) &&(strcmp(cmd,Identify_Struct[i].pCmd)))
        {
            strcpy(cmd,Identify_Struct[i].pCmd);
            return 1;
        }
        i++;
    }
    return 0;
} 

//根据命令确认是否是偏向A_PARTIAL_B或是混淆A_CONFUSE_B
int confuse_cmd_is_A2B(char* cmd)
{
    int group_id = 0;
    int legibility_flag_A = 0;
    int legibility_flag_B = 0;
    int i = 0;
    
    group_id = confuse_cmd_get_group_id(cmd);
    legibility_flag_A = confuse_cmd_get_legibility_flag(cmd);
    while(Identify_Struct[i].group_ID != 0xFF)
    {
        if((Identify_Struct[i].group_ID == group_id) &&(strcmp(cmd,Identify_Struct[i].pCmd)))
        {
            legibility_flag_B = Identify_Struct[i].legibility_flag;
            break;
        }
        i++;
    }
    
    if(legibility_flag_A == legibility_flag_B)
    {    
        return A_CONFUSE_B;
    }
    else
    {
        return A_PARTIAL_B;
    }
}
#else
Confusion_Identify_TypeDef Identify_Struct[]=
{
    {0xFF,}
};
int confuse_cmd_is_word(char* cmd)
{
    return NO_CONFUSE_CMD;
}

//根据命令词查找属组ID
int confuse_cmd_get_group_id(char* cmd)
{
    return INVALID_CONFUSE_ID;
}

//根据命令词查找易识别标记
int confuse_cmd_get_legibility_flag(char* cmd)
{
    return INVALID_CONFUSE_ID;
}

//根据命令词查找易混淆字位置
int confuse_cmd_get_identify_word_site(char* cmd)
{
    return INVALID_CONFUSE_ID;
}

//根据命令词查找成对的易混淆词
int confuse_cmd_get_pair_word(char* cmd)
{
    return 0;
} 

//根据命令确认是否是偏向A_PARTIAL_B或是混淆A_CONFUSE_B
int confuse_cmd_is_A2B(char* cmd)
{
    return A_PARTIAL_B;
}
#endif 
