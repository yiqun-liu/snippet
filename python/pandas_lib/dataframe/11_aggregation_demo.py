"""
Aggregation Demo
================

APIs covered:
- .groupby()          # Group by column(s)
- .agg()              # Aggregate using multiple functions
- .pivot_table()      # Create pivot table
- .value_counts()     # Count unique values
- .crosstab()         # Cross-tabulation
- .melt()             # Unpivot DataFrame
- .pivot()            # Pivot DataFrame

Key concepts:
- groupby() splits data into groups based on column values
- agg() applies aggregation functions to each group
- groupby + agg: most common pattern for summaries
- pivot_table() is like Excel pivot table

Task: Aggregate payment/income data by category and month.
"""

import pandas as pd


def basic_groupby():
    """Group by a single column."""
    df = pd.DataFrame(
        {
            "category": ["food", "transport", "food", "transport", "food"],
            "amount": [50.0, 100.0, 30.0, 80.0, 60.0],
        }
    )

    # Group by category
    grouped = df.groupby("category")
    print(f"Grouped object: {type(grouped)}\n")

    # Sum per category
    category_totals = grouped["amount"].sum()
    print(f"Sum by category:\n{category_totals}\n")

    # Multiple aggregations
    summary = grouped["amount"].agg(["sum", "mean", "count"])
    print(f"Summary by category:\n{summary}\n")


def groupby_with_columns():
    """Group by multiple columns."""
    df = pd.DataFrame(
        {
            "type": ["payment", "payment", "income", "income"],
            "category": ["food", "transport", "salary", "bonus"],
            "amount": [50.0, 100.0, 5000.0, 1000.0],
        }
    )

    # Group by type and category
    grouped = df.groupby(["type", "category"])["amount"].sum()
    print(f"Grouped by type and category:\n{grouped}\n")


def agg_with_dict():
    """Specify different aggregations per column."""
    df = pd.DataFrame(
        {
            "category": ["food", "transport", "food"],
            "amount": [50.0, 100.0, 30.0],
            "count": [1, 1, 1],
        }
    )

    # Different aggregations per column
    result = df.groupby("category").agg({"amount": "sum", "count": "sum"})
    print(f"Aggregated with dict:\n{result}\n")


def pivot_table():
    """Create pivot table (like Excel)."""
    df = pd.DataFrame(
        {
            "month": ["Jan", "Jan", "Feb", "Feb", "Jan"],
            "type": ["payment", "payment", "payment", "income", "income"],
            "amount": [50.0, 100.0, 30.0, 5000.0, 200.0],
        }
    )

    # Pivot table: rows=month, cols=type, values=amount
    pivot = pd.pivot_table(
        df, values="amount", index="month", columns="type", aggfunc="sum"
    )
    print(f"Pivot table:\n{pivot}\n")

    # Fill missing with 0
    pivot_filled = pd.pivot_table(
        df, values="amount", index="month", columns="type", aggfunc="sum", fill_value=0
    )
    print(f"Pivot table (filled):\n{pivot_filled}\n")


def value_counts():
    """Count frequency of each value."""
    df = pd.DataFrame({"category": ["food", "transport", "food", "food", "transport"]})

    # Count occurrences
    counts = df["category"].value_counts()
    print(f"Value counts:\n{counts}\n")


def crosstab():
    """Cross-tabulation (frequency table)."""
    df = pd.DataFrame(
        {
            "month": ["Jan", "Jan", "Feb", "Feb"],
            "category": ["food", "transport", "food", "transport"],
        }
    )

    # Cross-tab of month and category
    ct = pd.crosstab(df["month"], df["category"])
    print(f"Cross-tabulation:\n{ct}\n")


def melt_unpivot():
    """Unpivot wide DataFrame to long format."""
    df = pd.DataFrame(
        {"month": ["Jan", "Feb"], "food": [100.0, 150.0], "transport": [80.0, 90.0]}
    )

    # Melt to long format
    melted = pd.melt(
        df,
        id_vars=["month"],
        value_vars=["food", "transport"],
        var_name="category",
        value_name="amount",
    )
    print(f"Melted (long format):\n{melted}\n")


def pivot_wide():
    """Pivot long to wide format."""
    df = pd.DataFrame(
        {
            "month": ["Jan", "Jan", "Feb", "Feb"],
            "category": ["food", "transport", "food", "transport"],
            "amount": [100.0, 80.0, 150.0, 90.0],
        }
    )

    # Pivot to wide format
    pivoted = df.pivot(index="month", columns="category", values="amount").reset_index()
    print(f"Pivoted (wide format):\n{pivoted}\n")


if __name__ == "__main__":
    basic_groupby()
    groupby_with_columns()
    agg_with_dict()
    pivot_table()
    value_counts()
    crosstab()
    melt_unpivot()
    pivot_wide()
