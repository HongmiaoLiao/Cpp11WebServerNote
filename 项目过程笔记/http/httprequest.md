# http/httprequest.h、http/httprequest.cpp

## 使用库

* algorithm 头文件

```C++
#include <algorithm>

// search 函数，在first和last之间搜索[s_first, s_last)前闭后开的序列元素第一次出现的位置
template< class ForwardIt1, class ForwardIt2 >
ForwardIt1 search( ForwardIt1 first, ForwardIt1 last,
                   ForwardIt2 s_first, ForwardIt2 s_last );
```

* regex 头文件, 定义一个类模板来分析正则表达式 (C++)，以及定义多个类模板和函数以在文本中搜索正则表达式对象的匹配项。

```C++
#include <regex>

// regex 类型，char basic_regex 的类型定义。
typedef basic_regex<char> regex;

// smatch 类型，字符串 match_results 的类型定义。用于存储正则表达式匹配操作后在目标字符序列上找到的匹配项，每个匹配项都具有相应的sub_match类型。
typedef match_results<string::const_iterator> smatch;

// regex_match 函数， 确定正则表达式e是否与整个目标字符序列匹配，目标字符序列可以指定为std::string、C-string或迭代器对。这里指定为std::string
template <class IOtraits, class IOalloc, class Alloc, class Elem, class RXtraits, class Alloc2>
bool regex_match(
    const basic_string<Elem, IOtraits, IOalloc>& str,
    match_results<typename basic_string<Elem, IOtraits, IOalloc>::const_iterator, Alloc>& match,
    const basic_regex<Elem, RXtraits, Alloc2>& re,
    match_flag_type flags = match_default);
// 注意，regex_match将只成功地将正则表达式匹配到整个字符序列，而std::regex_search将成功地匹配子序列。


```

## 笔记


