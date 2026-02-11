"""
DataFrame Attributes Demo
==========================

APIs covered:
- .shape              # Tuple of (rows, columns)
- .dtypes             # Series of column data types
- .columns            # Index of column labels
- .index              # Index of row labels
- .values             # Underlying NumPy array
- .axes               # Both index and columns
- .ndim               # Number of dimensions (always 2 for DataFrame)
- .size               # Total number of elements

Task: Explore DataFrame properties and metadata.
"""

import pandas as pd


def explore_attributes():
    """Access and display DataFrame attributes."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20", "2024-02-05"],
            "amount": [100.0, 200.0, 50.0],
            "category": ["food", "transport", "food"],
        }
    )

    # Shape: (rows, columns)
    print(f"Shape: {df.shape}")  # (3, 3)

    # Data types for each column
    print(f"\nData types:\n{df.dtypes}")
    # date      object
    # amount    float64
    # category  object

    # Column labels
    print(f"\nColumns: {df.columns}")  # Index(['date', 'amount', 'category'])

    # Row index
    print(f"Index: {df.index}")  # RangeIndex(start=0, stop=3)

    # Underlying NumPy array (loses labels)
    print(f"\nValues (NumPy array):\n{df.values}")

    # Both axes
    print(f"\nAxes: {df.axes}")  # [Index([0, 1, 2]), Columns...]

    # Number of dimensions
    print(f"\nDimensions: {df.ndim}")  # 2

    # Total elements
    print(f"Size: {df.size}")  # 9 (3 rows × 3 columns)


def custom_index():
    """DataFrame with custom index."""
    df = pd.DataFrame(
        {"amount": [100.0, 200.0, 50.0], "category": ["food", "transport", "food"]},
        index=["p1", "p2", "p3"],
    )

    print(f"Custom index: {df.index}")  # Index(['p1', 'p2', 'p3'])
    print(f"Columns: {df.columns}")  # Index(['amount', 'category'])


if __name__ == "__main__":
    explore_attributes()
    print()
    custom_index()
