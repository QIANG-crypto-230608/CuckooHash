#include<iostream>
#include<vector>
#include<functional>
#include<cmath>
#include<algorithm>
#include<stdexcept>
#include<string>
#include<unordered_map>
#include<random>
#include<ctime>
#include<chrono>
using namespace std;

template<typename Key, typename Value>
class CuckooHash
{
private:
	vector<pair<Key, Value>> table1;//The first hash table
	vector<pair<Key, Value>> table2;//The second hash table
	hash<Key> hasher;//Hash function

	size_t size; //The size of the hash tables
	size_t maxInterations; //Maxium number of iterations during insert
	size_t currentSize; //Current number of elements in the hash table
	double loadFactorThreshold; //Load factor threshold for resizing

	unordered_map<Key, Value> stash; //Additional space to store elements that exceed maxium iterations

public:
	//Constructor
	CuckooHash(size_t size, double loadFactorThreshold = 0.75) :
		size(size), maxInterations(10), currentSize(0), loadFactorThreshold(loadFactorThreshold)
	{
		table1.resize(size);
		table2.resize(size);
	}

	//Insert key-value pair into the hash table
	void insert(const Key& key, const Value& value)
	{
		if (contains(key))
		{
			//If the key already exists, update the value
			update(key, value);
			return;
		}

		//Check if resizing is required
		if (currentSize >= size * loadFactorThreshold)
			rehash();

		//Try to insert the key-value pair
		if (insertInternal(key, value, 0))
		{
			++currentSize;
			cout << "Key inserted: " << key << std::endl;
		}
		else {
			stash[key] = value; //Insert the key-value pair into the stash
			cout << "Key stored in stash: " << key << endl;
		}
	}

	//Check if the hash table contains the specified key
	bool contains(const Key& key)
	{
		Value value;
		return find(key, value);
	}

	//Find the value associated with the specificed key
	bool find(const Key& key, Value& value)
	{
		size_t hash1 = hasher(key) % size; //Compute hash for table 1
		size_t hash2 = (hasher(key) / size) % size; //Compute hash for table 2

		//Check if the key exists in table 1
		if (table1[hash1].first == key)
		{
			value = table1[hash1].second;
			return true;
		}

		//Check if the key exists in table 2
		if (table2[hash2].first == key)
		{
			value = table2[hash2].second;
			return true;
		}

		return false; //Key not found
	}

	//Update the value associated with the specified key
	void update(const Key& key, const Value& value)
	{
		size_t hash1 = hasher(key) % size; //Compute hash for table 1
		size_t hash2 = (hasher(key) / size) % size; //Compute hash for table 2

		//Update value in table 1
		if (table1[hash1].first == key)
		{
			table1[hash1].second = value;
		}
		//Update value in table 2
		else if (table2[hash2].first == key)
		{
			table2[hash2].second = value;
		}
		else
		{
			stash[key] = value;
		}
	}

	//Erase the specified key from the hash table
	void erase(const Key& key)
	{
		size_t hash1 = hasher(key) % size; //Compute hash for table 1
		size_t hash2 = (hasher(key) / size) % size; //Compute hash for table 2

		//Erase key from table 1
		if (table1[hash1].first == key)
		{
			table1[hash1].first = Key{};
			table1[hash1].second = Value{};
			--currentSize;
			cout << "Key erased: " << key << endl;
		}
		//Erase key from table 1
		else if (table2[hash2].first == key)
		{
			table2[hash2].first = Key{};
			table2[hash2].second = Value{};
			--currentSize;
			cout << "Key erased: " << key << endl;
		}
		else
		{
			auto it = stash.find(key);
			if (it != stash.end())
			{
				stash.erase(it);
				cout << "Key erased from stash: " << key << endl;
			}
			else {
				throw runtime_error("Key not found.");
			}
		}
	}

private:
	//Internal function for inserting key-value pair into the hash table
	bool insertInternal(const Key& key, const Value& value, size_t iteration)
	{
		if (iteration >= maxInterations)
		{
			return false;
		}
		size_t hash1 = hasher(key) % size; //Compute hash for table 1
		size_t hash2 = (hasher(key) / size) % size; //Compute hash for table 2

		// Check if the key exists in table 1
			if (table1[hash1].first == key)
			{
				table1[hash1].second = value;
				return true;
			}

		//Check if the key exists in table 2
		if (table2[hash2].first == key)
		{
			table2[hash2].second = value;
			return true;
		}

		//Check if there is an empty slot in table 1
		if (table1[hash1].first == Key{})
		{
			table1[hash1] = make_pair(key, value);
			return true;
		}
		//Check if there is an empty slot in table 2
		if (table2[hash2].first == Key{})
		{
			table2[hash2] = make_pair(key, value);
			return true;
		}
		//Determine the current table and position to evict the existing key-value pair
		size_t currentTable = (iteration % 2 == 0) ? 1 : 2;
		size_t currentPos = (currentTable == 1) ? hash1 : hash2;
		Key currentKey = key;
		Value currentValue = value;

		//Swap the current key-value pair with the new key-value pair
		if (currentTable == 1)
		{
			swap(currentKey, table1[currentPos].first);
			swap(currentValue, table1[currentPos].second);
		}
		else {
			swap(currentKey, table2[currentPos].first);
			swap(currentValue, table2[currentPos].second);
		}

		//Recursively insert the evicted key-value pair
		return insertInternal(currentKey, currentValue, iteration + 1);
	}

	void rehash()
	{
		//Rehash the hash tables to a larger size
		size_t newSize = getNextSize();//Get the next size for the hash tables
		vector<pair<Key, Value>> newTable1(newSize);//Create a new hash table 1
		vector<pair<Key, Value>> newTable2(newSize);//Create a new hash table 2

		//Swap the existing hash tables with new hash tables
		swap(table1, newTable1);
		swap(table2, newTable2);

		size = newSize; //Update the size
		currentSize = 0; //Reset the current size

		//Re-insert the key-value pairs from the old hash tables to the new hash tables
		for (auto& entry : newTable1)
		{
			if (entry.first != Key{})
				insert(entry.first, entry.second);
		}

		for (auto& entry : newTable2)
		{
			if (entry.first != Key{})
				insert(entry.first, entry.second);
		}
	}

	//Get the next size for the hash tables
	size_t getNextSize()
	{
		size_t newSize = size * 2 + 1; 
		while (!isPrime(newSize))
			++newSize; //Find the next prime size
		return newSize;
	}

	//Check if a number is prime
	bool isPrime(size_t n)
	{
		if (n <= 1)
			return false;
		if (n <= 3)
			return true;
		if (n % 2 == 0 || n % 3 == 0)
			return false;
		for (size_t i = 5; i * i <= n; i += 6)
		{
			if (n % i == 0 || n % (i + 2) == 0)
				return false;
		}

		return true;
	}
};

int generateRandomNumber(int min, int max)
{
	/*
		1.default_random_engine: Random number engine is used to generate random numbers
		2.Use time(nullptr) to get the local time as the seed of the random number engine, to make sure the generated random number
		  sequence is different each time the program is run 
		3.uniform_int_distribution<int>: used to generate uniformly distributed random integers in a specifical range
		4.distribution(engine): To generate a random integer by random number engine and distributor
	*/
	static default_random_engine engine(static_cast<unsigned int>(time(nullptr))); 
	uniform_int_distribution<int> distribution(min, max);
	return distribution(engine);
}

int main(void)
{
	constexpr size_t TABLE_SIZE = 100; //Size of the hash tables

	CuckooHash<int, int> hashTable(TABLE_SIZE);

	//Fill the hash table with random values
	for (int i = 0; i < TABLE_SIZE * 2; ++i)
	{
		int key = generateRandomNumber(1, TABLE_SIZE * 10);
		int value = key * key;
		hashTable.insert(key, value);
	}

	//Find random keys in the hash table
	constexpr int NUM_QUERIES = 10;
	for (int i = 0; i < NUM_QUERIES; ++i)
	{
		int key = generateRandomNumber(1, TABLE_SIZE * 10);
		int value;
		if (hashTable.find(key, value))
			cout << "Found value for key " << key << ": " << value << endl;
		else
			cout << "Key " << key << " not found." << endl;
	}
}
//int main(void)
//{
//	CuckooHash<int, string> hashTable(10);
//	hashTable.insert(1, "Value 1");
//	hashTable.insert(2, "Value 2");
//	hashTable.insert(11, "Value 11");
//	hashTable.insert(21, "Value 21");
//	hashTable.insert(31, "Value 31");
//	hashTable.insert(41, "Value 41");
//
//	string value;
//	if (hashTable.find(1, value))
//		cout << "Found value: " << value << endl;
//	if (hashTable.contains(2))
//		cout << "Key 2 exists." << endl;
//
//	hashTable.erase(2);
//
//	if (!hashTable.contains(2))
//		cout << "Key 2 does not exist." << endl;
//
//	//Access the value in the stash
//	if (hashTable.find(3, value))
//		cout << "Find value in stash: " << value << endl;
//
//	return 0;
//}