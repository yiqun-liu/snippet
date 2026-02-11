# DataFrame Core API Demo

A hands-on exploration of pandas DataFrame operations using personal payment/income data.

## DataFrame

A DataFrame is a 2-dimensional labeled data structure with columns of potentially different types. Think of it as a spreadsheet or SQL table in Python.

```
                    date     amount  category
0          2024-01-15   100.00      food
1          2024-01-20   200.00   transport
2          2024-02-05    50.00      food
```

### Key Characteristics

| Feature | Description |
|---------|-------------|
| **2D Structure** | Rows and columns, like a spreadsheet |
| **Labeled Axes** | Both rows (index) and columns have labels |
| **Mixed Types** | Each column can have a different dtype |
| **Mutable** | Can modify data after creation |
| **Size Mutable** | Can add/remove columns |

## Core Concepts

### Index vs Columns

- **Index**: Row labels (leftmost column, often hidden)
- **Columns**: Column labels (header row)

```
        index →    0        1        2
        columns → date     amount   category
row 0            "2024-01-15"  100.00  "food"
row 1            "2024-01-20"  200.00  "transport"
row 2            "2024-02-05"  50.00  "food"
```

If index is not specified, it defaults to row numbers (RangeIndex).

### Series vs DataFrame

| | Series | DataFrame |
|---|--------|-----------|
| **Dimensionality** | 1D | 2D |
| **Structure** | Single column with index | Multiple columns |
| **Creation** | `pd.Series([1, 2, 3])` | `pd.DataFrame({'a': [1,2,3]})` |
| **Relationship** | Column of DataFrame | Collection of Series |

### Data Types (dtypes)

Pandas infers types from Python data:

| pandas dtype | Python/NumPy type | Use Case |
|--------------|-------------------|----------|
| `int64` | `np.int64` | Whole numbers |
| `float64` | `np.float64` | Decimals |
| `object` | `str` | Text strings |
| `datetime64` | `np.datetime64` | Dates/times |
| `bool` | `bool` | True/False |
| `category` | categorical | Limited set of values |

## Demo Files

| File | Topic | Key APIs |
|------|-------|----------|
| `01_creation_demo.py` | Creating DataFrames | `pd.DataFrame()`, `pd.Series()` |
| `02_attributes_demo.py` | DataFrame properties | `.shape`, `.dtypes`, `.columns`, `.index` |
| `03_row_selection_demo.py` | Selecting rows | `.loc[]`, `.iloc[]`, boolean indexing |
| `04_column_selection_demo.py` | Selecting columns | `df['col']`, `df[['col1', 'col2']]` |
| `05_cleaning_demo.py` | Data cleaning | `.fillna()`, `.dropna()`, `.astype()`, `.rename()` |
| `06_transformation_demo.py` | Transformations | `.apply()`, `.map()`, `.transform()` |
| `07_sorting_demo.py` | Sorting | `.sort_values()`, `.sort_index()` |
| `08_string_demo.py` | String operations | `.str.lower()`, `.str.contains()`, `.str.split()` |
| `09_datetime_demo.py` | DateTime operations | `.to_datetime()`, `.dt` accessor |
| `10_statistics_demo.py` | Statistics | `.describe()`, `.mean()`, `.sum()` |
| `11_aggregation_demo.py` | Aggregations | `.groupby()`, `.agg()`, `.pivot_table()` |

## Running the Demos

```bash
cd python/pandas/dataframe
python 01_creation_demo.py
python 02_attributes_demo.py
# ... continue with each demo
```

## Learning Path

1. **Start with creation** - Understand how data becomes a DataFrame
2. **Learn attributes** - Explore structure and metadata
3. **Master selection** - Accessing data is fundamental
4. **Practice cleaning** - Real data is always messy
5. **Explore transformations** - Modify and reshape data
6. **Advanced operations** - Grouping, aggregation, pivoting

## Review Progress

01 done.
