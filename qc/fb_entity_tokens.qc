// SPDX-License-Identifier: GPL-2.0-or-later
// Entity/command writers use escaped quotes and backslashes; FTE's default
// tokenizer uses doubled quotes instead. Normalize explicitly without changing
// unknown backslash sequences in legacy MAP paths.
int fb_entity_tokenize(string source)
{
    // Ordinary quoted fields need no escape normalization or temporary copies.
    if(strstrofs(source,"\\",0)<0)return tokenize_console(source);
    string normalized="";int cursor=0,length=strlen(source);
    while(cursor<length) {
        while(cursor<length && str2chr(source,cursor)<=32)cursor++;
        if(cursor>=length)break;
        string quote=substring(source,cursor,1),value="";
        if(quote=="\"" || quote=="'") {
            cursor++;
            while(cursor<length) {
                string ch=substring(source,cursor++,1);
                if(ch==quote)break;
                if(ch=="\\" && cursor<length) {
                    string next=substring(source,cursor,1);
                    if(next=="\\" || next==quote){ch=next;cursor++;}
                }
                value=strcat(value,ch);
            }
        }else {
            int start=cursor;while(cursor<length && str2chr(source,cursor)>32)cursor++;
            value=substring(source,start,cursor-start);
        }
        value=strreplace("\\","\\\\",value);value=strreplace("\"","\\\"",value);value=strreplace("\n","\\n",value);
        normalized=strcat(normalized,"\\\"",value,"\" ");
    }
    return tokenize_console(normalized);
}
