#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// ==================== 被测试的函数声明 ====================
#define MAX_FILE_SIZE 1000000
#define N_GRAM 3
#define MAX_NGRAMS 50000
#define HASH_TABLE_SIZE 100003

typedef struct NGramNode
{
    char gram[N_GRAM + 1];
    int count;
    struct NGramNode *next;
} NGramNode;

typedef struct
{
    NGramNode **table;
    int size;
} HashTable;

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

// ==================== 测试统计 ====================
typedef struct
{
    int total;
    int passed;
    int failed;
} TestStats;

TestStats test_stats = {0, 0, 0};

// ==================== 测试断言宏 ====================
#define TEST_ASSERT(condition, message)              \
    do                                               \
    {                                                \
        test_stats.total++;                          \
        if (condition)                               \
        {                                            \
            test_stats.passed++;                     \
            printf("✓ PASS: %s\n", message);         \
        }                                            \
        else                                         \
        {                                            \
            test_stats.failed++;                     \
            printf("✗ FAIL: %s\n", message);         \
            printf("  Condition: %s\n", #condition); \
        }                                            \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual, message)                   \
    do                                                                 \
    {                                                                  \
        test_stats.total++;                                            \
        if ((expected) == (actual))                                    \
        {                                                              \
            test_stats.passed++;                                       \
            printf("✓ PASS: %s\n", message);                           \
        }                                                              \
        else                                                           \
        {                                                              \
            test_stats.failed++;                                       \
            printf("✗ FAIL: %s\n", message);                           \
            printf("  Expected: %d, Got: %d\n", (expected), (actual)); \
        }                                                              \
    } while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual, message) \
    do                                                      \
    {                                                       \
        test_stats.total++;                                 \
        if (strcmp(expected, actual) == 0)                  \
        {                                                   \
            test_stats.passed++;                            \
            printf("✓ PASS: %s\n", message);                \
        }                                                   \
        else                                                \
        {                                                   \
            test_stats.failed++;                            \
            printf("✗ FAIL: %s\n", message);                \
            printf("  Expected: \"%s\"\n", expected);       \
            printf("  Got: \"%s\"\n", actual);              \
        }                                                   \
    } while (0)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual, message)                 \
    do                                                                     \
    {                                                                      \
        test_stats.total++;                                                \
        if (fabs((expected) - (actual)) < 0.001f)                          \
        {                                                                  \
            test_stats.passed++;                                           \
            printf("✓ PASS: %s\n", message);                               \
        }                                                                  \
        else                                                               \
        {                                                                  \
            test_stats.failed++;                                           \
            printf("✗ FAIL: %s\n", message);                               \
            printf("  Expected: %.3f, Got: %.3f\n", (expected), (actual)); \
        }                                                                  \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr, message)   \
    do                                       \
    {                                        \
        test_stats.total++;                  \
        if ((ptr) != NULL)                   \
        {                                    \
            test_stats.passed++;             \
            printf("✓ PASS: %s\n", message); \
        }                                    \
        else                                 \
        {                                    \
            test_stats.failed++;             \
            printf("✗ FAIL: %s\n", message); \
            printf("  Pointer is NULL\n");   \
        }                                    \
    } while (0)

// ==================== 测试用例函数 ====================

// 测试1: 标点符号去除功能
void test_remove_punctuation()
{
    printf("\n=== 测试标点符号去除 ===\n");

    char test_str1[] = "Hello, World!";
    char expected1[] = "Hello World";
    remove_punctuation(test_str1);
    TEST_ASSERT_EQUAL_STRING(expected1, test_str1, "英文标点去除");

    char test_str2[] = "你好，世界！";
    char expected2[] = "你好世界";
    remove_punctuation(test_str2);
    TEST_ASSERT_EQUAL_STRING(expected2, test_str2, "中文标点去除");

    char test_str3[] = "测试123,456!789";
    char expected3[] = "测试123456789";
    remove_punctuation(test_str3);
    TEST_ASSERT_EQUAL_STRING(expected3, test_str3, "数字和标点混合");
}

// 测试2: 小写转换功能
void test_to_lower_case()
{
    printf("\n=== 测试小写转换 ===\n");

    char test_str1[] = "Hello WORLD";
    char expected1[] = "hello world";
    to_lower_case(test_str1);
    TEST_ASSERT_EQUAL_STRING(expected1, test_str1, "英文大小写转换");

    char test_str2[] = "HELLO World 中文";
    char expected2[] = "hello world 中文";
    to_lower_case(test_str2);
    TEST_ASSERT_EQUAL_STRING(expected2, test_str2, "中英文混合大小写转换");
}

// 测试3: 哈希表创建和释放
void test_hash_table_creation()
{
    printf("\n=== 测试哈希表操作 ===\n");

    HashTable *ht = create_hash_table(10);
    TEST_ASSERT_NOT_NULL(ht, "哈希表创建");
    TEST_ASSERT_NOT_NULL(ht->table, "哈希表数组分配");
    TEST_ASSERT_EQUAL(10, ht->size, "哈希表大小正确");

    free_hash_table(ht);
    printf("✓ 哈希表释放成功\n");
}

// 测试4: 哈希函数测试
void test_hash_function()
{
    printf("\n=== 测试哈希函数 ===\n");

    unsigned int hash1 = hash_function("abc", 100);
    unsigned int hash2 = hash_function("abc", 100);
    TEST_ASSERT_EQUAL(hash1, hash2, "相同字符串哈希值相同");

    unsigned int hash3 = hash_function("abc", 100);
    unsigned int hash4 = hash_function("abcd", 100);
    TEST_ASSERT(hash3 != hash4, "不同字符串哈希值不同");
}

// 测试5: n-gram生成测试
void test_ngram_generation()
{
    printf("\n=== 测试n-gram生成 ===\n");

    HashTable *ht = create_hash_table(100);
    char text[] = "abcdef";

    generate_ngrams(text, ht);

    int count = 0;
    for (int i = 0; i < ht->size; i++)
    {
        NGramNode *current = ht->table[i];
        while (current != NULL)
        {
            count++;
            current = current->next;
        }
    }

    TEST_ASSERT_EQUAL(4, count, "6字符文本生成4个3-gram");
    free_hash_table(ht);
}

// 测试6: 短文本n-gram测试
void test_short_text_ngram()
{
    printf("\n=== 测试短文本n-gram ===\n");

    HashTable *ht = create_hash_table(100);
    char text[] = "ab"; // 长度小于N_GRAM

    generate_ngrams(text, ht);

    int count = 0;
    for (int i = 0; i < ht->size; i++)
    {
        NGramNode *current = ht->table[i];
        while (current != NULL)
        {
            count++;
            current = current->next;
        }
    }

    TEST_ASSERT_EQUAL(0, count, "短文本不生成n-gram");
    free_hash_table(ht);
}

// 测试7: 完全相同文本的相似度
void test_identical_similarity()
{
    printf("\n=== 测试完全相同文本 ===\n");

    HashTable *ht1 = create_hash_table(100);
    HashTable *ht2 = create_hash_table(100);

    addhash(ht1, "abc");
    addhash(ht1, "def");
    addhash(ht2, "abc");
    addhash(ht2, "def");

    float similarity = calculate_jaccard_similarity(ht1, ht2);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, similarity, "完全相同文本相似度为1.0");

    free_hash_table(ht1);
    free_hash_table(ht2);
}

// 测试8: 完全不同文本的相似度
void test_completely_different()
{
    printf("\n=== 测试完全不同文本 ===\n");

    HashTable *ht1 = create_hash_table(100);
    HashTable *ht2 = create_hash_table(100);

    addhash(ht1, "abc");
    addhash(ht1, "def");
    addhash(ht2, "xyz");
    addhash(ht2, "uvw");

    float similarity = calculate_jaccard_similarity(ht1, ht2);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, similarity, "完全不同文本相似度为0.0");

    free_hash_table(ht1);
    free_hash_table(ht2);
}

// 测试9: 部分相似文本的相似度
void test_partial_similarity()
{
    printf("\n=== 测试部分相似文本 ===\n");

    HashTable *ht1 = create_hash_table(100);
    HashTable *ht2 = create_hash_table(100);

    addhash(ht1, "abc");
    addhash(ht1, "def");
    addhash(ht1, "ghi");
    addhash(ht2, "abc");
    addhash(ht2, "xyz");
    addhash(ht2, "uvw");

    float similarity = calculate_jaccard_similarity(ht1, ht2);
    TEST_ASSERT_EQUAL_FLOAT(0.2f, similarity, "部分相似文本测试");

    free_hash_table(ht1);
    free_hash_table(ht2);
}

// 测试10: 空文本相似度
void test_empty_text_similarity()
{
    printf("\n=== 测试空文本 ===\n");

    HashTable *ht1 = create_hash_table(100);
    HashTable *ht2 = create_hash_table(100);

    float similarity = calculate_jaccard_similarity(ht1, ht2);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, similarity, "空文本相似度为0.0");

    free_hash_table(ht1);
    free_hash_table(ht2);
}

// 测试11: 哈希表计数功能
void test_hash_table_counting()
{
    printf("\n=== 测试哈希表计数 ===\n");

    HashTable *ht = create_hash_table(100);

    addhash(ht, "abc");
    addhash(ht, "abc");
    addhash(ht, "abc");
    addhash(ht, "def");

    // 查找abc节点
    unsigned int index = hash_function("abc", 100);
    NGramNode *node = ht->table[index];
    while (node != NULL && strcmp(node->gram, "abc") != 0)
    {
        node = node->next;
    }

    TEST_ASSERT_NOT_NULL(node, "查找存在的n-gram");
    TEST_ASSERT_EQUAL(3, node->count, "重复添加计数正确");

    free_hash_table(ht);
}

// ==================== 主测试函数 ====================
int main()
{
    printf("开始单元测试...\n");
    printf("====================\n");

    // 运行所有测试用例
    test_remove_punctuation();
    test_to_lower_case();
    test_hash_table_creation();
    test_hash_function();
    test_ngram_generation();
    test_short_text_ngram();
    test_identical_similarity();
    test_completely_different();
    test_partial_similarity();
    test_empty_text_similarity();
    test_hash_table_counting();

    // 输出测试结果
    printf("\n====================\n");
    printf("测试完成！\n");
    printf("总计测试: %d\n", test_stats.total);
    printf("通过: %d\n", test_stats.passed);
    printf("失败: %d\n", test_stats.failed);
    printf("通过率: %.1f%%\n", (test_stats.passed * 100.0) / test_stats.total);

    if (test_stats.failed == 0)
    {
        printf("🎉 所有测试通过！\n");
        return 0;
    }
    else
    {
        printf("❌ 有测试失败，请检查代码\n");
        return 1;
    }
}

// ==================== 被测试函数的实现 ====================
// 这里需要包含你的实际函数实现
// 或者将你的main.c文件中的函数实现复制到这里

void remove_punctuation(char *str)
{
    char *src = str;
    char *dst = str;

    while (*src)
    {
        // 检查是否是ASCII字母、数字或空格
        if (isalnum((unsigned char)*src) || *src == ' ')
        {
            *dst++ = *src;
            src++;
        }
        // 检查是否是中文字符（UTF-8编码）
        else if ((unsigned char)*src & 0x80)
        {
            // 获取完整的UTF-8字符
            unsigned char byte1 = (unsigned char)*src;
            unsigned char byte2 = (unsigned char)*(src + 1);

            // 常见中文标点符号的UTF-8编码范围
            // 中文逗号：EFBC8C, 中文句号：EFBC8E, 中文感叹号：EFBC81
            // 中文冒号：EFBC9A, 中文分号：EFBC9B, 中文问号：EFBC9F
            if (byte1 == 0xEF && byte2 == 0xBC)
            {
                // 这是中文标点，跳过3个字节
                src += 3;
            }
            else
            {
                // 这是中文字符，保留3个字节
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
            }
        }
        else
        {
            // 跳过ASCII标点符号
            src++;
        }
    }
    *dst = '\0';
}

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

HashTable *create_hash_table(int size)
{
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    ht->size = size;
    ht->table = (NGramNode **)calloc(size, sizeof(NGramNode *));
    return ht;
}

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