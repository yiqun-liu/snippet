"""
Row Selection Demo
==================

APIs covered:
- .loc[]              # Label-based selection (uses index/column labels)
- .iloc[]             # Integer position-based selection (uses 0-based positions)
- Boolean indexing    # Filter using conditions

Key concepts:
- .loc[] selects by LABEL (index value, column name)
- .iloc[] selects by INTEGER POSITION (0, 1, 2...)
- Boolean indexing creates a mask (True/False) for filtering

Task: Select and filter rows using various methods.
"""

import pandas as pd


def loc_selection():
    """Select rows by label using .loc[]."""
    df = pd.DataFrame(
        {
            "amount": [100.0, 200.0, 50.0, 300.0],
            "category": ["food", "transport", "food", "entertainment"],
        },
        index=["p1", "p2", "p3", "p4"],
    )

    # Select single row by index label
    row_p1 = df.loc["p1"]
    print(f"Row p1:\n{row_p1}\n")

    # Select multiple rows by labels
    rows = df.loc[["p1", "p3"]]
    print(f"Rows p1 and p3:\n{rows}\n")

    # Select rows with slice (inclusive of endpoints)
    slice_rows = df.loc["p1":"p3"]
    print(f"Rows p1 to p3:\n{slice_rows}\n")


def iloc_selection():
    """Select rows by integer position using .iloc[]."""
    df = pd.DataFrame(
        {
            "amount": [100.0, 200.0, 50.0, 300.0],
            "category": ["food", "transport", "food", "entertainment"],
        }
    )

    # Select single row by position
    first_row = df.iloc[0]
    print(f"First row:\n{first_row}\n")

    # Select multiple rows by positions
    rows = df.iloc[[0, 2]]
    print(f"Rows at positions 0 and 2:\n{rows}\n")

    # Select with slice (like Python list slicing)
    slice_rows = df.iloc[1:3]
    print(f"Rows at positions 1 and 2:\n{slice_rows}\n")


def boolean_indexing():
    """Filter rows using boolean conditions."""
    df = pd.DataFrame(
        {
            "amount": [100.0, 200.0, 50.0, 300.0],
            "category": ["food", "transport", "food", "entertainment"],
        }
    )

    # Condition: amount > 80
    condition = df["amount"] > 80
    print(f"Boolean mask:\n{condition}\n")

    # Apply mask to filter
    filtered = df[df["amount"] > 80]
    print(f"Filtered (amount > 80):\n{filtered}\n")

    # Multiple conditions (use & for AND, | for OR)
    filtered = df[(df["amount"] > 80) & (df["category"] == "food")]
    print(f"Filtered (amount > 80 AND category == 'food'):\n{filtered}\n")

    # Using .loc[] with boolean mask
    filtered = df.loc[df["amount"] > 80]
    print(f"Same result with .loc[]:\n{filtered}\n")


def iloc_with_columns():
    """Select specific rows and columns with .iloc[]."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20", "2024-02-05"],
            "amount": [100.0, 200.0, 50.0],
            "category": ["food", "transport", "food"],
            "note": ["coffee", "bus", "snack"],
        }
    )

    # iloc[row_positions, column_positions]
    subset = df.iloc[0:2, 0:2]
    print(f"First 2 rows, first 2 columns:\n{subset}\n")

    # Select all rows, specific columns
    subset = df.iloc[:, [1, 2]]
    print(f"All rows, columns 1 and 2:\n{subset}\n")


if __name__ == "__main__":
    loc_selection()
    iloc_selection()
    boolean_indexing()
    iloc_with_columns()
