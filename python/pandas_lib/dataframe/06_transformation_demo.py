"""
Data Transformation Demo
========================

APIs covered:
- .apply()             # Apply function along axis (row or column)
- .map()               # Apply element-wise function to Series
- .applymap()          # Apply element-wise function to DataFrame
- .transform()         # Apply function that produces same-shape output
- .assign()            # Add new columns (chainable)
- .copy()              # Create a copy of DataFrame

Key concepts:
- .apply() works on rows/columns or Series
- .map() is for Series only (element-wise)
- .applymap() is for DataFrames (element-wise)
- .transform() must return same shape as input
- Use .copy() to avoid SettingWithCopyWarning

Task: Transform payment/income data using various methods.
"""

import pandas as pd


def apply_on_series():
    """Use .apply() on a Series."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0]})

    # Apply function to each element
    doubled = df["amount"].apply(lambda x: x * 2)
    print(f"Doubled:\n{doubled}\n")

    # Apply string method
    categories = pd.Series(["food", "transport", "FOOD"])
    upper = categories.apply(lambda x: x.upper())
    print(f"Uppercase:\n{upper}\n")


def apply_on_dataframe():
    """Use .apply() on DataFrame (axis parameter matters)."""
    df = pd.DataFrame({"amount": [100.0, 200.0], "count": [2, 1]})

    # Apply function to each column (axis=0, default)
    column_sums = df.apply(sum)
    print(f"Column sums:\n{column_sums}\n")

    # Apply function to each row (axis=1)
    row_sums = df.apply(lambda row: row["amount"] + row["count"], axis=1)
    print(f"Row sums:\n{row_sums}\n")


def map_series():
    """Use .map() for Series element-wise operations."""
    df = pd.DataFrame({"category": ["food", "transport", "entertainment"]})

    # Map using dictionary (value substitution)
    mapping = {
        "food": "Food & Dining",
        "transport": "Transportation",
        "entertainment": "Entertainment",
    }
    mapped = df["category"].map(mapping)
    print(f"Mapped categories:\n{mapped}\n")


def transform_demo():
    """Use .transform() for same-shape transformations."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0]})

    # Transform must return same length
    transformed = df["amount"].transform(lambda x: x * 2)
    print(f"Transformed:\n{transformed}\n")

    # Multiple transforms
    result = df.transform({"original": lambda x: x, "doubled": lambda x: x * 2})
    print(f"Multiple transforms:\n{result}\n")


def assign_columns():
    """Add new columns using .assign()."""
    df = pd.DataFrame(
        {"amount": [100.0, 200.0, 50.0], "category": ["food", "transport", "food"]}
    )

    # Chain assignments
    result = (
        df.assign(amount_usd=df["amount"])
        .assign(amount_jpy=lambda x: x["amount"] * 150)
        .assign(is_food=lambda x: x["category"] == "food")
    )
    print(f"With assigned columns:\n{result}\n")


def copy_dataframe():
    """Create a copy to avoid modifying original."""
    df = pd.DataFrame({"amount": [100.0, 200.0]})

    # Create copy
    df_copy = df.copy()

    # Modify copy (original unchanged)
    df_copy["amount"] = df_copy["amount"] * 2
    print(f"Original:\n{df}\n")
    print(f"Copy (modified):\n{df_copy}\n")


if __name__ == "__main__":
    apply_on_series()
    apply_on_dataframe()
    map_series()
    transform_demo()
    assign_columns()
    copy_dataframe()
