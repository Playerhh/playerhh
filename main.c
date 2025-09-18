/**
 * 论文查重系统 - 基于n-gram和Jaccard相似度算法
 * 作者: playerhh
 * 功能: 计算两个文本文件的相似度（重复率）
 * 输入: 原文文件路径, 抄袭版文件路径, 输出文件路径
 * 输出: 相似度分数（0.00-1.00）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// ==================== 常量定义 ====================
#define MAX_FILE_SIZE 1000000  // 定义最大文件大小为1MB（约可存储50万字中文文本）
#define N_GRAM 3               // 定义使用3-gram算法（即3个字符为一个文本片段）
#define MAX_NGRAMS 50000       // 定义最大n-gram数量限制，防止内存溢出
#define HASH_TABLE_SIZE 100003 // 哈希表大小（使用质数100003可以减少哈希冲突）

// ==================== 数据结构定义 ====================

/**
 * n-gram节点结构体
 * 用于存储每个n-gram片段及其出现次数
 */
typedef struct NGramNode
{
    char gram[N_GRAM + 1];  // 存储n-gram字符串，+1是为了存放字符串结束符'\0'
    int count;              // 记录该n-gram在文本中出现的次数
    struct NGramNode *next; // 指向下一个节点的指针，用于处理哈希冲突（链表法）
} NGramNode;

/**
 * 哈希表结构体
 * 用于高效存储和查找n-gram，提高查询效率
 */
typedef struct
{
    NGramNode **table; // 哈希表数组，每个元素是指向NGramNode的指针
    int size;          // 哈希表的大小（槽位数量）
} HashTable;

// ==================== 函数声明 ====================
// 函数原型声明，确保编译器知道这些函数的存在
void remove_punctuation(char *str);                                                    // 去除标点符号
void to_lower_case(char *str);                                                         // 转换为小写
HashTable *create_hash_table(int size);                                                // 创建哈希表
void free_hash_table(HashTable *ht);                                                   // 释放哈希表内存
unsigned int hash_function(const char *str, int table_size);                           // 哈希函数
void addhash(HashTable *ht, const char *gram);                                         // 添加元素到哈希表
int get_intersection_count(HashTable *ht1, HashTable *ht2);                            // 计算交集数量
int get_union_count(HashTable *ht1, HashTable *ht2);                                   // 计算并集数量
float calculate_jaccard_similarity(HashTable *ht_original, HashTable *ht_plagiarized); // 计算相似度
void generate_ngrams(const char *text, HashTable *ht);                                 // 生成n-gram

// ==================== 主函数 ====================
/**
 * 程序主入口
 * @param argc 命令行参数个数（包括程序名本身）
 * @param argv 命令行参数数组（字符串数组）
 * @return 程序退出状态码（0表示成功，非0表示失败）
 */
int main(int argc, char *argv[])
{
    // 检查命令行参数数量是否正确（应该是程序名+3个文件路径=4个参数）
    if (argc != 4)
    {
        printf("错误: 参数数量不正确！\n");
        printf("使用方法: %s <原文文件> <抄袭版文件> <输出文件>\n", argv[0]);
        printf("示例: main.exe C:\\orig.txt C:\\orig_add.txt C:\\ans.txt\n");
        return 1; // 返回错误状态码，表示程序异常退出
    }

    // 解析命令行参数，将参数保存到对应的变量中
    char *original_file = argv[1];    // 第一个参数：原文文件的绝对路径
    char *plagiarized_file = argv[2]; // 第二个参数：抄袭版文件的绝对路径
    char *output_file = argv[3];      // 第三个参数：输出答案文件的绝对路径

    // 打印处理信息，方便用户了解程序运行状态
    printf("开始处理文件...\n");
    printf("原文文件: %s\n", original_file);
    printf("抄袭版文件: %s\n", plagiarized_file);
    printf("输出文件: %s\n", output_file);

    // ========== 读取原文文件 ==========
    FILE *file = fopen(original_file, "r"); // 以只读模式打开原文文件
    if (file == NULL)
    { // 检查文件是否成功打开
        printf("错误：无法打开原文文件: %s\n", original_file);
        printf("请检查文件路径是否正确，文件是否存在\n");
        return 1; // 文件打开失败，退出程序
    }

    char original_text[MAX_FILE_SIZE];                                      // 定义字符数组来存储原文内容
    size_t original_len = fread(original_text, 1, MAX_FILE_SIZE - 1, file); // 读取文件内容
    original_text[original_len] = '\0';                                     // 在字符串末尾添加结束符
    fclose(file);                                                           // 关闭文件，释放文件资源
    printf("原文读取完成，长度: %zu 字符\n", original_len);                 // 打印原文长度信息

    // ========== 读取抄袭版文件 ==========
    file = fopen(plagiarized_file, "r"); // 以只读模式打开抄袭版文件
    if (file == NULL)
    { // 检查文件是否成功打开
        printf("错误：无法打开抄袭版文件: %s\n", plagiarized_file);
        return 1; // 文件打开失败，退出程序
    }

    char plagiarized_text[MAX_FILE_SIZE];                                         // 定义字符数组来存储抄袭版内容
    size_t plagiarized_len = fread(plagiarized_text, 1, MAX_FILE_SIZE - 1, file); // 读取文件内容
    plagiarized_text[plagiarized_len] = '\0';                                     // 在字符串末尾添加结束符
    fclose(file);                                                                 // 关闭文件
    printf("抄袭版读取完成，长度: %zu 字符\n", plagiarized_len);                  // 打印抄袭版长度信息

    // ========== 文本预处理 ==========
    printf("正在进行文本预处理...\n");
    to_lower_case(original_text);         // 将原文中的英文字母统一转换为小写
    to_lower_case(plagiarized_text);      // 将抄袭版中的英文字母统一转换为小写
    remove_punctuation(original_text);    // 去除原文中的标点符号
    remove_punctuation(plagiarized_text); // 去除抄袭版中的标点符号

    // ========== 创建哈希表存储n-gram ==========
    printf("生成n-gram特征...\n");
    // 创建两个哈希表，分别存储原文和抄袭版的n-gram特征
    HashTable *ht_original = create_hash_table(HASH_TABLE_SIZE);
    HashTable *ht_plagiarized = create_hash_table(HASH_TABLE_SIZE);

    // 为两个文本生成n-gram特征并存入对应的哈希表
    generate_ngrams(original_text, ht_original);
    generate_ngrams(plagiarized_text, ht_plagiarized);

    // ========== 计算相似度 ==========
    printf("计算相似度...\n");
    // 计算Jaccard相似度，得到0.0-1.0之间的相似度分数
    float similarity = calculate_jaccard_similarity(ht_original, ht_plagiarized);

    // ========== 输出结果到文件 ==========
    file = fopen(output_file, "w"); // 以写入模式打开输出文件
    if (file == NULL)
    { // 检查文件是否成功创建
        printf("错误：无法创建输出文件: %s\n", output_file);
        // 释放已分配的内存，避免内存泄漏
        free_hash_table(ht_original);
        free_hash_table(ht_plagiarized);
        return 1; // 文件创建失败，退出程序
    }
    fprintf(file, "%.2f\n", similarity); // 将相似度写入文件，保留两位小数
    fclose(file);                        // 关闭输出文件

    // ========== 释放内存 ==========
    // 释放两个哈希表占用的内存
    free_hash_table(ht_original);
    free_hash_table(ht_plagiarized);

    // 打印最终结果信息
    printf("查重完成！重复率: %.2f%%\n", similarity * 100);
    printf("结果已保存到: %s\n", output_file);

    return 0; // 程序正常退出
}

// ==================== 工具函数实现 ====================

/**
 * 去除字符串中的标点符号和特殊字符
 * 只保留字母、数字、汉字和空格
 * @param str 要处理的字符串（原地修改）
 */
void remove_punctuation(char *str)
{
    char *src = str; // 源指针，用于读取原始字符串
    char *dst = str; // 目标指针，用于写入处理后的字符串

    // 遍历字符串中的每个字符
    while (*src)
    {
        // 判断字符是否需要保留：
        // isalnum: 检查是否是字母或数字
        // *src == ' ': 检查是否是空格
        // (*src & 0x80): 检查是否是中文字符（ASCII码大于127）
        if (isalnum((unsigned char)*src) || *src == ' ' || (*src & 0x80))
        {
            *dst++ = *src; // 保留该字符，并移动目标指针
        }
        src++; // 移动源指针到下一个字符
    }
    *dst = '\0'; // 在处理后的字符串末尾添加结束符
}

/**
 * 将英文字母转换为小写（不影响中文字符）
 * @param str 要处理的字符串（原地修改）
 */
void to_lower_case(char *str)
{
    // 遍历字符串中的每个字符
    for (int i = 0; str[i]; i++)
    {
        // 检查字符是否是大写字母（A-Z）
        if (str[i] >= 'A' && str[i] <= 'Z')
        {
            str[i] = str[i] + 32; // 转换为小写（ASCII码加32）
        }
        // 中文字符（ASCII码大于127）保持不变
    }
}

/**
 * 创建哈希表
 * @param size 哈希表大小
 * @return 新创建的哈希表指针
 */
HashTable *create_hash_table(int size)
{
    // 分配HashTable结构体的内存
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    ht->size = size; // 设置哈希表大小
    // 分配哈希表数组的空间，并用calloc初始化为NULL
    ht->table = (NGramNode **)calloc(size, sizeof(NGramNode *));
    return ht; // 返回创建的哈希表
}

/**
 * 释放哈希表及其所有节点的内存
 * @param ht 要释放的哈希表
 */
void free_hash_table(HashTable *ht)
{
    // 遍历哈希表的所有槽位
    for (int i = 0; i < ht->size; i++)
    {
        NGramNode *current = ht->table[i]; // 获取当前槽位的链表头节点
        // 遍历链表，释放所有节点
        while (current != NULL)
        {
            NGramNode *temp = current; // 保存当前节点指针
            current = current->next;   // 移动到下一个节点
            free(temp);                // 释放当前节点内存
        }
    }
    // 释放哈希表数组和结构体本身的内存
    free(ht->table);
    free(ht);
}

/**
 * 哈希函数：将字符串映射到哈希表索引
 * 使用DJB2哈希算法，具有良好的分布特性
 * @param str 输入字符串
 * @param table_size 哈希表大小
 * @return 哈希值（0 到 table_size-1）
 */
unsigned int hash_function(const char *str, int table_size)
{
    unsigned int hash = 5381; // DJB2算法的初始魔术数
    int c;                    // 当前字符
    // 遍历字符串中的每个字符
    while ((c = *str++))
    {
        // hash * 33 + c 的优化计算方式：(hash << 5) + hash = hash * 32 + hash = hash * 33
        hash = ((hash << 5) + hash) + c;
    }
    return hash % table_size; // 取模确保索引在有效范围内
}

/**
 * 向哈希表添加n-gram或增加计数
 * @param ht 目标哈希表
 * @param gram 要添加的n-gram字符串
 */
void addhash(HashTable *ht, const char *gram)
{
    // 计算n-gram的哈希值，确定在哈希表中的位置
    unsigned int index = hash_function(gram, ht->size);

    // 在哈希槽的链表中查找是否已存在相同的n-gram
    NGramNode *current = ht->table[index];
    while (current != NULL)
    {
        if (strcmp(current->gram, gram) == 0)
        {                     // 找到相同的n-gram
            current->count++; // 增加计数
            return;           // 直接返回，不需要创建新节点
        }
        current = current->next; // 移动到链表的下一个节点
    }

    // 创建新节点（使用头插法）
    NGramNode *new_node = (NGramNode *)malloc(sizeof(NGramNode));
    strncpy(new_node->gram, gram, N_GRAM); // 复制n-gram字符串
    new_node->gram[N_GRAM] = '\0';         // 确保字符串正确结束
    new_node->count = 1;                   // 初始化计数为1
    new_node->next = ht->table[index];     // 新节点指向原链表头
    ht->table[index] = new_node;           // 更新链表头为新节点
}

/**
 * 计算两个哈希表的交集数量（共同n-gram的最小计数之和）
 * @param ht1 第一个哈希表
 * @param ht2 第二个哈希表
 * @return 交集数量
 */
int get_intersection_count(HashTable *ht1, HashTable *ht2)
{
    int intersection = 0; // 交集数量计数器

    // 遍历第一个哈希表的所有槽位
    for (int i = 0; i < ht1->size; i++)
    {
        NGramNode *current = ht1->table[i]; // 获取当前槽位的链表头
        // 遍历当前槽位的链表
        while (current != NULL)
        {
            // 计算当前n-gram在第二个哈希表中的位置
            unsigned int index = hash_function(current->gram, ht2->size);
            NGramNode *temp = ht2->table[index]; // 获取第二个哈希表对应槽位的链表

            // 在第二个哈希表的链表中查找相同的n-gram
            while (temp != NULL)
            {
                if (strcmp(current->gram, temp->gram) == 0)
                { // 找到相同的n-gram
                    // 取两个计数中的较小值（交集原则）
                    intersection += (current->count < temp->count) ? current->count : temp->count;
                    break; // 找到后跳出内层循环
                }
                temp = temp->next; // 移动到下一个节点
            }
            current = current->next; // 移动到下一个节点
        }
    }

    return intersection; // 返回交集数量
}

/**
 * 计算两个哈希表的并集数量（所有n-gram计数之和）
 * @param ht1 第一个哈希表
 * @param ht2 第二个哈希表
 * @return 并集数量（需要减去交集）
 */
int get_union_count(HashTable *ht1, HashTable *ht2)
{
    int union_count = 0; // 并集数量计数器

    // 统计第一个哈希表的总计数
    for (int i = 0; i < ht1->size; i++)
    {
        NGramNode *current = ht1->table[i];
        while (current != NULL)
        {
            union_count += current->count; // 累加计数
            current = current->next;       // 移动到下一个节点
        }
    }

    // 统计第二个哈希表的总计数
    for (int i = 0; i < ht2->size; i++)
    {
        NGramNode *current = ht2->table[i];
        while (current != NULL)
        {
            union_count += current->count; // 累加计数
            current = current->next;       // 移动到下一个节点
        }
    }

    return union_count; // 返回总计数（后面需要减去交集部分）
}

/**
 * 计算Jaccard相似度系数
 * Jaccard相似度 = 交集大小 / 并集大小
 * @param ht_original 原文的n-gram哈希表
 * @param ht_plagiarized 抄袭版的n-gram哈希表
 * @return 相似度分数（0.0-1.0）
 */
float calculate_jaccard_similarity(HashTable *ht_original, HashTable *ht_plagiarized)
{
    // 计算两个文本共有的n-gram数量（交集）
    int intersection = get_intersection_count(ht_original, ht_plagiarized);
    // 计算两个文本所有的n-gram数量总和
    int union_total = get_union_count(ht_original, ht_plagiarized);

    // 减去交集部分（因为在两个表中都被计算了一次）
    union_total -= intersection;

    // 避免除零错误（如果两个文本都为空）
    if (union_total == 0)
    {
        return 0.0f; // 返回0相似度
    }

    // 计算并返回Jaccard相似度
    return (float)intersection / union_total;
}

/**
 * 从文本生成n-gram并存储到哈希表
 * @param text 输入文本（已预处理）
 * @param ht 目标哈希表
 */
void generate_ngrams(const char *text, HashTable *ht)
{
    int len = strlen(text); // 获取文本长度

    // 使用滑动窗口生成所有可能的n-gram
    for (int i = 0; i <= len - N_GRAM; i++)
    {
        char gram[N_GRAM + 1];           // 临时存储n-gram的数组
        strncpy(gram, text + i, N_GRAM); // 从文本中复制N_GRAM个字符
        gram[N_GRAM] = '\0';             // 添加字符串结束符
        addhash(ht, gram);               // 将n-gram添加到哈希表
    }
}