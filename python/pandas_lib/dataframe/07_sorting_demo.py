"""
Sorting Demo
============

APIs covered:
- .sort_values()      # Sort by column values
- .sort_index()       # Sort by index
- ascending           # Sort order (True/False)
- inplace             # Modify original (True/False)
- na_position         # Where to put NaN values ('first'/'last')
- by                  # Column(s) to sort by (multiple)

Key concepts:
- sort_values() sorts by column values
- sort_index() sorts by row index
- Returns new DataFrame by default (use inplace=True to modify)
- Multiple columns: first sorts by first column, then second

Task: Sort payment/income data by various criteria.
"""

import pandas as pd


def sort_by_single_column():
    """Sort by a single column."""
    df = pd.DataFrame(
        {"amount": [100.0, 50.0, 200.0], "category": ["food", "food", "transport"]}
    )

    # Sort by amount (ascending, default)
    sorted_df = df.sort_values("amount")
    print(f"Sorted by amount (ascending):\n{sorted_df}\n")

    # Sort by amount (descending)
    sorted_df = df.sort_values("amount", ascending=False)
    print(f"Sorted by amount (descending):\n{sorted_df}\n")


def sort_by_multiple_columns():
    """Sort by multiple columns."""
    df = pd.DataFrame(
        {
            "category": ["food", "transport", "food", "transport"],
            "amount": [200.0, 100.0, 50.0, 150.0],
        }
    )

    # Sort by category first, then amount
    sorted_df = df.sort_values(["category", "amount"])
    print(f"Sorted by category, then amount:\n{sorted_df}\n")

    # Different sort orders per column
    sorted_df = df.sort_values(["category", "amount"], ascending=[True, False])
    print(f"Sorted: category asc, amount desc:\n{sorted_df}\n")


def sort_with_na():
    """Handle missing values in sorting."""
    df = pd.DataFrame(
        {"amount": [100.0, None, 50.0], "category": ["food", None, "transport"]}
    )

    # NaN at last
    sorted_df = df.sort_values("amount", na_position="last")
    print(f"NaN last:\n{sorted_df}\n")

    # NaN at first
    sorted_df = df.sort_values("amount", na_position="first")
    print(f"NaN first:\n{sorted_df}\n")


def sort_by_index():
    """Sort by row index."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0]}, index=["c", "a", "b"])

    # Sort index (alphabetically)
    sorted_df = df.sort_index()
    print(f"Sorted by index:\n{sorted_df}\n")


def inplace_sorting():
    """Modify DataFrame in place."""
    df = pd.DataFrame({"amount": [100.0, 50.0, 200.0]})

    # Sort and modify original
    df.sort_values("amount", inplace=True)
    print(f"After inplace sort:\n{df}\n")


if __name__ == "__main__":
    sort_by_single_column()
    sort_by_multiple_columns()
    sort_with_na()
    sort_by_index()
    inplace_sorting()
