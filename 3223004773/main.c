/**
 * 论文查重系统 - 基于n-gram和Jaccard相似度算法
 * 作者: playerhh
 * 功能: 计算两个文本文件的相似度（重复率）
 * 输入: 原文文件路径, 抄袭版文件路径, 输出文件路径
 * 输出: 相似度分数（0.00-1.00）
 */
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_FILE_SIZE 1000000
#define N_GRAM 3
#define MAX_NGRAMS 50000
#define HASH_TABLE_SIZE 100003

/**
 * n-gram节点结构体
 * 用于存储每个n-gram片段及其出现次数
 */
typedef struct NGramNode
{
    char gram[N_GRAM + 1];
    int count;
    struct NGramNode *next;
} NGramNode;

/**
 * 哈希表结构体
 * 用于高效存储和查找n-gram，提高查询效率
 */
typedef struct
{
    NGramNode **table;
    int size;
} HashTable;

// 函数声明
void remove_punctuation(char *str);
void to_lower_case(char *str);
HashTable *create_hash_table(int size);
void free_hash_table(HashTable *ht);
unsigned int hash_function(const char *str, int table_size);
void addhash(HashTable *ht, const char *gram);
int get_intersection_count(HashTable *ht1, HashTable *ht2);
int get_union_count(HashTable *ht1, HashTable *ht2);
float calculate_jaccard_similarity(HashTable *ht_original, HashTable *ht_plagiarized);
void generate_ngrams(const char *text, HashTable *ht);

/**
 * 程序主入口
 * @param argc 命令行参数个数（包括程序名本身）
 * @param argv 命令行参数数组（字符串数组）
 * @return 程序退出状态码（0表示成功，非0表示失败）
 */
int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("错误: 参数数量不正确！\n");
        printf("使用方法: %s <原文文件> <抄袭版文件> <输出文件>\n", argv[0]);
        return 1;
    }

    char *original_file = argv[1];
    char *plagiarized_file = argv[2];
    char *output_file = argv[3];

    // 读取原文文件
    FILE *file = fopen(original_file, "r");
    if (file == NULL)
    {
        printf("错误：无法打开原文文件: %s\n", original_file);
        return 1;
    }

    char original_text[MAX_FILE_SIZE];
    size_t original_len = fread(original_text, 1, MAX_FILE_SIZE - 1, file);
    original_text[original_len] = '\0';
    fclose(file);

    // 读取抄袭版文件
    file = fopen(plagiarized_file, "r");
    if (file == NULL)
    {
        printf("错误：无法打开抄袭版文件: %s\n", plagiarized_file);
        return 1;
    }

    char plagiarized_text[MAX_FILE_SIZE];
    size_t plagiarized_len = fread(plagiarized_text, 1, MAX_FILE_SIZE - 1, file);
    plagiarized_text[plagiarized_len] = '\0';
    fclose(file);

    // 文本预处理：转换为小写并去除标点
    to_lower_case(original_text);
    to_lower_case(plagiarized_text);
    remove_punctuation(original_text);
    remove_punctuation(plagiarized_text);

    // 创建哈希表存储n-gram特征
    HashTable *ht_original = create_hash_table(HASH_TABLE_SIZE);
    HashTable *ht_plagiarized = create_hash_table(HASH_TABLE_SIZE);

    // 生成n-gram特征
    generate_ngrams(original_text, ht_original);
    generate_ngrams(plagiarized_text, ht_plagiarized);

    // 计算Jaccard相似度
    float similarity = calculate_jaccard_similarity(ht_original, ht_plagiarized);

    // 输出结果到文件
    file = fopen(output_file, "w");
    if (file == NULL)
    {
        printf("错误：无法创建输出文件: %s\n", output_file);
        free_hash_table(ht_original);
        free_hash_table(ht_plagiarized);
        return 1;
    }
    fprintf(file, "%.2f\n", similarity);
    fclose(file);

    // 释放内存
    free_hash_table(ht_original);
    free_hash_table(ht_plagiarized);

    printf("查重完成！重复率: %.2f%%\n", similarity * 100);
    return 0;
}

/**
 * 去除字符串中的标点符号和特殊字符
 * 只保留字母、数字、汉字和空格
 * @param str 要处理的字符串（原地修改）
 */
void remove_punctuation(char *str)
{
    char *src = str;
    char *dst = str;

    while (*src)
    {
        if (isalnum((unsigned char)*src) || *src == ' ')
        {
            *dst++ = *src;
            src++;
        }
        else if ((unsigned char)*src & 0x80)
        {
            unsigned char byte1 = (unsigned char)*src;
            unsigned char byte2 = (unsigned char)*(src + 1);

            if (byte1 == 0xEF && byte2 == 0xBC)
            {
                src += 3;
            }
            else
            {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
            }
        }
        else
        {
            src++;
        }
    }
    *dst = '\0';
}

/**
 * 将英文字母转换为小写（不影响中文字符）
 * @param str 要处理的字符串（原地修改）
 */
void to_lower_case(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
        {
            str[i] = str[i] + 32;
        }
    }
}

/**
 * 创建哈希表
 * @param size 哈希表大小
 * @return 新创建的哈希表指针
 */
HashTable *create_hash_table(int size)
{
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    ht->size = size;
    ht->table = (NGramNode **)calloc(size, sizeof(NGramNode *));
    return ht;
}

/**
 * 释放哈希表及其所有节点的内存
 * @param ht 要释放的哈希表
 */
void free_hash_table(HashTable *ht)
{
    for (int i = 0; i < ht->size; i++)
    {
        NGramNode *current = ht->table[i];
        while (current != NULL)
        {
            NGramNode *temp = current;
            current = current->next;
            free(temp);
        }
    }
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
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % table_size;
}

/**
 * 向哈希表添加n-gram或增加计数
 * @param ht 目标哈希表
 * @param gram 要添加的n-gram字符串
 */
void addhash(HashTable *ht, const char *gram)
{
    unsigned int index = hash_function(gram, ht->size);

    NGramNode *current = ht->table[index];
    while (current != NULL)
    {
        if (strcmp(current->gram, gram) == 0)
        {
            current->count++;
            return;
        }
        current = current->next;
    }

    NGramNode *new_node = (NGramNode *)malloc(sizeof(NGramNode));
    strncpy(new_node->gram, gram, N_GRAM);
    new_node->gram[N_GRAM] = '\0';
    new_node->count = 1;
    new_node->next = ht->table[index];
    ht->table[index] = new_node;
}

/**
 * 计算两个哈希表的交集数量（共同n-gram的最小计数之和）
 * @param ht1 第一个哈希表
 * @param ht2 第二个哈希表
 * @return 交集数量
 */
int get_intersection_count(HashTable *ht1, HashTable *ht2)
{
    int intersection = 0;

    for (int i = 0; i < ht1->size; i++)
    {
        NGramNode *current = ht1->table[i];
        while (current != NULL)
        {
            unsigned int index = hash_function(current->gram, ht2->size);
            NGramNode *temp = ht2->table[index];

            while (temp != NULL)
            {
                if (strcmp(current->gram, temp->gram) == 0)
                {
                    intersection += (current->count < temp->count) ? current->count : temp->count;
                    break;
                }
                temp = temp->next;
            }
            current = current->next;
        }
    }

    return intersection;
}

/**
 * 计算两个哈希表的并集数量（所有n-gram计数之和）
 * @param ht1 第一个哈希表
 * @param ht2 第二个哈希表
 * @return 并集数量（需要减去交集）
 */
int get_union_count(HashTable *ht1, HashTable *ht2)
{
    int union_count = 0;

    for (int i = 0; i < ht1->size; i++)
    {
        NGramNode *current = ht1->table[i];
        while (current != NULL)
        {
            union_count += current->count;
            current = current->next;
        }
    }

    for (int i = 0; i < ht2->size; i++)
    {
        NGramNode *current = ht2->table[i];
        while (current != NULL)
        {
            union_count += current->count;
            current = current->next;
        }
    }

    return union_count;
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
    int intersection = get_intersection_count(ht_original, ht_plagiarized);
    int union_total = get_union_count(ht_original, ht_plagiarized);

    union_total -= intersection;

    if (union_total == 0)
    {
        return 0.0f;
    }

    return (float)intersection / union_total;
}

/**
 * 从文本生成n-gram并存储到哈希表
 * @param text 输入文本（已预处理）
 * @param ht 目标哈希表
 */
void generate_ngrams(const char *text, HashTable *ht)
{
    int len = strlen(text);

    for (int i = 0; i <= len - N_GRAM; i++)
    {
        char gram[N_GRAM + 1];
        strncpy(gram, text + i, N_GRAM);
        gram[N_GRAM] = '\0';
        addhash(ht, gram);
    }
}