#include <string>
#include <stdio.h>
#include <isocline.h>
#include "deepseek.hpp"

int main()
{
        DeepSeek ds;
        std::string input;
        char* buffer;

        while (1){
                buffer = ic_readline(">>");
                if (!buffer){
                        printf("输入失败\n");
                        break;
                }
                input = buffer;
                free(buffer);
                try{
                        ds.input(input);
                }catch (const char* msg){
                        break;
                }
        }

        return 0;
}