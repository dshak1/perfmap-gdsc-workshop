#include "perfmap/hash_map.h"

#include "gtest/gtest.h"

namespace perfmap {
namespace {

#define PERFMAP_IGNORE_STATUS(expr) (void)(expr)

struct ConstantHash {
  size_t operator()(int) const { return 0; }
};

TEST(HashMapStarterTest, DefaultConstructedIsEmpty) {
  HashMap<int, int> map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0u);
}

TEST(HashMapStarterTest, InsertAndFindWork) {
  HashMap<int, int> map;
  ASSERT_TRUE(map.Insert(1, 10).ok());
  ASSERT_TRUE(map.Insert(2, 20).ok());

  ASSERT_TRUE(map.Find(1).ok());
  ASSERT_TRUE(map.Find(2).ok());
  EXPECT_EQ(*map.Find(1), 10);
  EXPECT_EQ(*map.Find(2), 20);
}

TEST(HashMapStarterTest, InsertUpdatesExistingKey) {
  HashMap<int, int> map;
  PERFMAP_IGNORE_STATUS(map.Insert(7, 70));
  PERFMAP_IGNORE_STATUS(map.Insert(7, 700));

  ASSERT_TRUE(map.Find(7).ok());
  EXPECT_EQ(*map.Find(7), 700);
  EXPECT_EQ(map.size(), 1u);
}

TEST(HashMapStarterTest, EraseRemovesExistingKey) {
  HashMap<int, int> map;
  PERFMAP_IGNORE_STATUS(map.Insert(1, 10));
  ASSERT_TRUE(map.Erase(1).ok());
  EXPECT_FALSE(map.Find(1).ok());
}

TEST(HashMapStarterTest, DeleteShouldNotBreakProbeChain) {
  HashMap<int, int, ConstantHash> map(4);
  PERFMAP_IGNORE_STATUS(map.Insert(0, 100));
  PERFMAP_IGNORE_STATUS(map.Insert(4, 400));
  PERFMAP_IGNORE_STATUS(map.Insert(8, 800));

  ASSERT_TRUE(map.Erase(4).ok());

  // TODO(workshop): this should pass after erase uses tombstones correctly.
  ASSERT_TRUE(map.Find(8).ok());
  EXPECT_EQ(*map.Find(8), 800);
}

TEST(HashMapStarterTest, RehashPreservesEntries) {
  HashMap<int, int> map(4);
  for (int i = 0; i < 64; ++i) {
    ASSERT_TRUE(map.Insert(i, i * 3).ok());
  }

  // TODO(workshop): once growth is added, this should keep all entries alive.
  for (int i = 0; i < 64; ++i) {
    ASSERT_TRUE(map.Find(i).ok());
    EXPECT_EQ(*map.Find(i), i * 3);
  }
}

}  // namespace
}  // namespace perfmap
