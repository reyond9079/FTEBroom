// SPDX-License-Identifier: GPL-2.0-or-later
// Visibility queries can stop at any occluder. Picking and physics retain full traces.
int(vector start,vector end) ftebroom_line_visible = #0:ftebroom_line_visible;
var float autocvar_ftebroom_native_labels=1;
int fb_label_native_traces,fb_label_full_traces;
float fb_label_trace_seconds,fb_label_draw_seconds;
int fb_label_visible(vector start,vector end,int node)
{
    if(autocvar_ftebroom_native_labels && checkbuiltin(ftebroom_line_visible)){
        fb_label_native_traces++;return ftebroom_line_visible(start,end);
    }
    fb_label_full_traces++;traceline(start,end,TRUE,world);return trace_fraction>=.999;
}
