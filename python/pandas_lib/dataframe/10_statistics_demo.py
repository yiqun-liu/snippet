"""
Statistics Demo
===============

APIs covered:
- .describe()         # Summary statistics (count, mean, std, min, 25%, 50%, 75%, max)
- .mean()             # Arithmetic mean
- .sum()              # Sum of values
- .count()            # Non-NA count
- .min()              # Minimum
- .max()              # Maximum
- .median()           # Median (50% percentile)
- .std()              # Standard deviation
- .var()              # Variance
- .quantile()         # Specific percentile
- .cumsum()           # Cumulative sum
- .cummin()           # Cumulative minimum
- .idxmax()           # Index of maximum value
- .idxmin()           # Index of minimum value
- .corr()             # Correlation between columns

Key concepts:
- Statistics skip NaN values by default
- axis parameter: 0 for columns, 1 for rows
- describe() returns summary for numeric columns

Task: Compute statistics on payment/income amounts.
"""

import pandas as pd


def describe_summary():
    """Get comprehensive summary statistics."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0, 300.0, 150.0]})

    summary = df["amount"].describe()
    print(f"Summary statistics:\n{summary}\n")

    # Describe entire DataFrame
    print(f"DataFrame describe:\n{df.describe()}\n")


def basic_aggregations():
    """Basic statistical functions."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0]})

    print(f"Mean: {df['amount'].mean()}")
    print(f"Sum: {df['amount'].sum()}")
    print(f"Count: {df['amount'].count()}")
    print(f"Min: {df['amount'].min()}")
    print(f"Max: {df['amount'].max()}")
    print(f"Median: {df['amount'].median()}")
    print(f"Std: {df['amount'].std()}")
    print(f"Variance: {df['amount'].var()}\n")


def quantile():
    """Calculate specific percentiles."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0, 300.0, 150.0]})

    # Specific quantile
    q50 = df["amount"].quantile(0.5)
    print(f"50th percentile (median): {q50}\n")

    # Multiple quantiles
    quantiles = df["amount"].quantile([0.25, 0.5, 0.75])
    print(f"25th, 50th, 75th percentiles:\n{quantiles}\n")


def cumulative_operations():
    """Cumulative operations."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0, 300.0]})

    df["cumsum"] = df["amount"].cumsum()
    df["cummin"] = df["amount"].cummin()
    print(f"Cumulative operations:\n{df}\n")


def index_of_min_max():
    """Find index of min/max values."""
    df = pd.DataFrame({"amount": [100.0, 200.0, 50.0, 300.0]})

    max_idx = df["amount"].idxmax()
    min_idx = df["amount"].idxmin()
    print(f"Index of max: {max_idx}")
    print(f"Index of min: {min_idx}")
    print(f"Row with max amount:\n{df.loc[max_idx]}\n")


def correlation():
    """Calculate correlation between columns."""
    df = pd.DataFrame(
        {"payment": [100.0, 200.0, 50.0], "income": [5000.0, 5200.0, 5100.0]}
    )

    corr = df["payment"].corr(df["income"])
    print(f"Correlation: {corr}\n")


def row_wise_statistics():
    """Compute statistics across columns for each row."""
    df = pd.DataFrame({"amount1": [100.0, 200.0], "amount2": [50.0, 150.0]})

    # Mean across columns for each row
    row_means = df.mean(axis=1)
    print(f"Row means:\n{row_means}\n")


if __name__ == "__main__":
    describe_summary()
    basic_aggregations()
    quantile()
    cumulative_operations()
    index_of_min_max()
    correlation()
    row_wise_statistics()
