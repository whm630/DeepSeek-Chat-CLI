#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#define CONFIG "/.deepseek/config"
/*
配置JSON字段
{
        "api_key": "sk-xxxxxxxxxxxxxx",
        "model": "deepseek-xxxxx",
        "thinking_type": "enabled"/"disabled"
}
*/

static void menu(cJSON* config_json);
static cJSON* get_config();
static char* s_gets(char* str,int n);
static void save_config(cJSON* config_json);

int main()
{
        cJSON* config_json;
        char c,buffer[256];

        config_json = get_config();
        if (!config_json){
                fprintf(stderr,"\033[31m~/.deepseek不存在或配置文件已损坏！\033[0m\n");
                return 0;
        }
        do{
                menu(config_json);
                c = getchar();
                while (getchar() != '\n');
                switch (c){
                case 'a':
                        printf("输入api_key:");
                        s_gets(buffer,256);
                        cJSON_DeleteItemFromObject(config_json,"api_key");
                        cJSON_AddStringToObject(config_json,"api_key",buffer);
                        break;
                case 'b':
                        puts("a)deepseek-flash(目前主力)        b)deepseek-v4-pro(即将淘汰)");
                        printf(">>");
                        c = getchar();
                        while (getchar() != '\n');
                        cJSON_DeleteItemFromObject(config_json,"model");
                        cJSON_AddStringToObject(config_json,"model",(c == 'a')?"deepseek-flash":"deepseek-v4-pro");
                        if (c == 'q')
                                c = '\0';
                        break;
                case 'c':
                        puts("a)开启           b)关闭");
                        printf(">>");
                        c = getchar();
                        while (getchar() != '\n');
                        cJSON_DeleteItemFromObject(config_json,"thinking_type");
                        cJSON_AddStringToObject(config_json,"thinking_type",(c == 'a')?"enabled":"disabled");
                        if (c == 'q')
                                c = '\0';
                        break;
                case 'q':
                        break;
                default:
                        puts("错误选项");
                }
        }while(c != 'q');
        save_config(config_json);
        cJSON_Delete(config_json);

        return 0;
}

static void menu(cJSON* config_json)
{
        cJSON* index;

        printf("\033[34mDeepSeek\033[0m CLI Config\n");
        index = cJSON_GetObjectItem(config_json,"api_key");
        if (cJSON_IsString(index) && index->valuestring)
                printf("api_key:已设置\n");
        else
                printf("api_key:未设置\n");
        index = cJSON_GetObjectItem(config_json,"model");
        if (cJSON_IsString(index) && index->valuestring)
                printf("模型:%s\n",index->valuestring);
        else
                printf("模型:未设置\n");
        index = cJSON_GetObjectItem(config_json,"thinking_type");
        if (cJSON_IsString(index) && index->valuestring)
                printf("思考模式:%s\n",index->valuestring);
        else
                printf("思考模式:未设置\n");
        puts("a)设置api_key                 b)设置模型");
        puts("c)设置思考模式                 q)保存并退出");
        printf(">>");
}

static cJSON* get_config()
{
        char* home = getenv("HOME");
        char *config_path,*buffer;
        cJSON* config_json;
        FILE* fp;
        size_t size;

        config_path = (char*)malloc(strlen(home)+strlen(CONFIG)+1);
        strcpy(config_path,home);
        strcat(config_path,CONFIG);
        if ((fp = fopen(config_path,"r")) == NULL){
                if ((fp = fopen(config_path,"w")) == NULL)
                        return NULL;
                config_json = cJSON_CreateObject();
                fclose(fp);
                return config_json;
        }
        free(config_path);
        fseek(fp,0L,SEEK_END);
        size = ftell(fp);
        fseek(fp,0L,SEEK_SET);
        buffer = (char*)malloc(size+1);
        fread(buffer,size,1,fp);
        buffer[size] = '\0';
        config_json = cJSON_Parse(buffer);
        free(buffer);
        fclose(fp);

        return config_json;
}

static char* s_gets(char* str,int n)
{
        char *p,*index;

        if ((p = fgets(str,n,stdin)) == NULL)
                return NULL;
        index = p;
        while (*index){
                if (*index == '\n'){
                        *index = '\0';
                        break;
                }
                index++;
        }

        return p;
}

static void save_config(cJSON* config_json)
{
        FILE* fp;
        char *home,*config_path,*buffer;

        home = getenv("HOME");
        config_path = (char*)malloc(strlen(home)+strlen(CONFIG)+1);
        strcpy(config_path,home);
        strcat(config_path,CONFIG);
        fp = fopen(config_path,"w");
        free(config_path);
        buffer = cJSON_Print(config_json);
        fwrite(buffer,strlen(buffer)+1,1,fp);
        free(buffer);
        fclose(fp);
}