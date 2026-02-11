"""
Column Selection Demo
=====================

APIs covered:
- df['column']              # Select single column (returns Series)
- df[['col1', 'col2']]      # Select multiple columns (returns DataFrame)
- df.column_name            # Attribute access (less common, may conflict)
- .filter()                 # Select columns by pattern

Key concepts:
- Single column access returns a Series
- Multiple column access returns a DataFrame
- Column names with spaces/special chars require bracket notation

Task: Select and filter columns using various methods.
"""

import pandas as pd


def single_column():
    """Select a single column (returns Series)."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20"],
            "amount": [100.0, 200.0],
            "category": ["food", "transport"],
        }
    )

    # Single column -> Series
    amounts = df["amount"]
    print(f"Type: {type(amounts)}")
    print(f"Amounts Series:\n{amounts}\n")


def multiple_columns():
    """Select multiple columns (returns DataFrame)."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20"],
            "amount": [100.0, 200.0],
            "category": ["food", "transport"],
            "note": ["coffee", "bus"],
        }
    )

    # Multiple columns -> DataFrame
    subset = df[["date", "amount"]]
    print(f"Type: {type(subset)}")
    print(f"Subset DataFrame:\n{subset}\n")


def column_order():
    """Select columns in specific order."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20"],
            "amount": [100.0, 200.0],
            "category": ["food", "transport"],
        }
    )

    # Reorder columns
    reordered = df[["category", "amount", "date"]]
    print(f"Reordered:\n{reordered}\n")


def filter_columns():
    """Select columns matching a pattern."""
    df = pd.DataFrame(
        {
            "payment_date": ["2024-01-15", "2024-01-20"],
            "payment_amount": [100.0, 200.0],
            "income_date": ["2024-01-01", "2024-02-01"],
            "income_amount": [5000.0, 5200.0],
        }
    )

    # Select columns containing "payment"
    payment_cols = df.filter(like="payment")
    print(f"Columns with 'payment':\n{payment_cols}\n")

    # Select columns matching regex (starts with "date")
    date_cols = df.filter(regex="^date")
    print(f"Columns starting with 'date':\n{date_cols}\n")


def conditional_column_selection():
    """Select columns based on their content or properties."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20"],
            "amount": [100.0, 200.0],
            "category": ["food", "transport"],
        }
    )

    # Select numeric columns
    numeric_df = df.select_dtypes(include=["number"])
    print(f"Numeric columns:\n{numeric_df}\n")

    # Select string/object columns
    object_df = df.select_dtypes(include=["object"])
    print(f"Object columns:\n{object_df}\n")


if __name__ == "__main__":
    single_column()
    multiple_columns()
    column_order()
    filter_columns()
    conditional_column_selection()
