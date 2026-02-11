"""
Data Cleaning Demo
==================

APIs covered:
- .isna()              # Detect missing values (NaN, None, NaT)
- .notna()             # Opposite of isna()
- .fillna()            # Replace missing values with specified value
- .dropna()            # Remove rows/columns with missing values
- .astype()            # Convert column to specific data type
- .rename()            # Rename columns or index
- .replace()           # Replace values (not just NaN)
- .drop_duplicates()   # Remove duplicate rows
- .duplicated()        # Identify duplicate rows

Key concepts:
- Missing values: NaN (float), None (object), NaT (datetime)
- fillna() doesn't modify original (unless inplace=True)
- astype() can fail if conversion is impossible
- rename() maps old names to new names

Task: Clean and standardize payment/income data.
"""

import pandas as pd
import numpy as np


def handle_missing_values():
    """Detect and handle missing values."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20", None],
            "amount": [100.0, np.nan, 50.0],
            "category": ["food", "transport", "food"],
        }
    )

    # Detect missing values
    print(f"isna():\n{df.isna()}\n")

    # Count missing per column
    print(f"Missing counts:\n{df.isna().sum()}\n")

    # Fill missing values
    filled = df.fillna({"date": "2024-02-01", "amount": 0.0})
    print(f"Filled:\n{filled}\n")

    # Drop rows with any missing value
    cleaned = df.dropna()
    print(f"Dropped NaN rows:\n{cleaned}\n")


def type_conversion():
    """Convert column data types."""
    df = pd.DataFrame(
        {
            "amount": ["100", "200", "50"],  # Strings
            "date": ["2024-01-15", "2024-01-20", "2024-02-05"],
        }
    )

    # Convert string to float
    df["amount"] = df["amount"].astype(float)
    print(f"amount dtype: {df['amount'].dtype}\n")

    # Convert date string to datetime
    df["date"] = pd.to_datetime(df["date"])
    print(f"date dtype: {df['date'].dtype}\n")
    print(f"DataFrame:\n{df}\n")


def rename_columns():
    """Rename columns."""
    df = pd.DataFrame({"amt": [100.0, 200.0], "cat": ["food", "transport"]})

    # Rename using dictionary
    renamed = df.rename(columns={"amt": "amount", "cat": "category"})
    print(f"Renamed:\n{renamed}\n")

    # Rename using function
    upper = df.rename(columns=str.upper)
    print(f"Uppercase columns:\n{upper}\n")


def replace_values():
    """Replace specific values."""
    df = pd.DataFrame({"category": ["food", "transport", "FOOD", "entertainment"]})

    # Replace specific values
    replaced = df.replace({"FOOD": "food"})
    print(f"Replaced 'FOOD' with 'food':\n{replaced}\n")

    # Replace with dictionary mapping multiple values
    replaced = df.replace({"food": "Food", "transport": "Transport"})
    print(f"All replaced:\n{replaced}\n")


def handle_duplicates():
    """Remove duplicate rows."""
    df = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-15", "2024-01-20"],
            "amount": [100.0, 100.0, 200.0],
            "category": ["food", "food", "transport"],
        }
    )

    # Identify duplicates
    print(f"Duplicated (True for duplicate rows):\n{df.duplicated()}\n")

    # Drop duplicates (keep first occurrence)
    deduped = df.drop_duplicates()
    print(f"Deduplicated:\n{deduped}\n")


if __name__ == "__main__":
    handle_missing_values()
    type_conversion()
    rename_columns()
    replace_values()
    handle_duplicates()
