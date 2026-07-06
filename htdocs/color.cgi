#!/usr/bin/perl -w

use strict;
use CGI;

my($cgi) = new CGI;

print $cgi->header('text/html');

# 获取颜色参数，如果没有就默认蓝色
my($color) = "blue";
$color = $cgi->param('color') if defined $cgi->param('color');

# 1. 正常生成 HTML 头部，不要加 -BGCOLOR
print $cgi->start_html(-title => uc($color));

# 2. 手动插入一段 CSS 代码来设置背景色（完美兼容所有现代浏览器）
print "<style>body { background-color: $color; }</style>\n";

print $cgi->h1("Welcome to Color Demo");
print $cgi->p("The current background color is: <b>$color</b>");

# 3. 生成表单
print $cgi->start_form;
print "Enter a color: ";
print $cgi->textfield('color'); # 这里的 'color' 就是参数名
print $cgi->submit('Submit', 'Change Color');
print $cgi->end_form;

print $cgi->hr;
print $cgi->end_html;