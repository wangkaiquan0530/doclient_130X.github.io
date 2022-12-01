#ifndef __NEED_IDENTIFY_H
#define __NEED_IDENTIFY_H
#endif

typedef struct 
{
    int         group_ID;
    const char* pCmd;
    int         legibility_flag;
    int         identify_word_site;
}Confusion_Identify_TypeDef;

int confuse_cmd_is_word(char* cmd);
int confuse_cmd_get_group_id(char* cmd);
int confuse_cmd_get_legibility_flag(char* cmd);
int confuse_cmd_get_identify_word_site(char* cmd);
int confuse_cmd_is_A2B(char* cmd);
int confuse_cmd_get_pair_word(char* cmd);

#define INVALID_CONFUSE_ID  (0xFF)
#define A_PARTIAL_B         (1)
#define A_CONFUSE_B         (0)
#define IS_CONFUSE_CMD      (1)
#define NO_CONFUSE_CMD      (0)


