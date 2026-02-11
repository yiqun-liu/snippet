"""
DateTime Operations Demo
========================

APIs covered:
- pd.to_datetime()     # Convert to datetime
- .dt.year            # Extract year
- .dt.month           # Extract month
- .dt.day             # Extract day
- .dt.dayofweek       # Day of week (0=Monday)
- .dt.day_name()      # Day name (Monday, Tuesday...)
- .dt.month_name()    # Month name
- .dt.quarter         # Quarter number
- .dt.is_month_start  # Is first day of month
- .dt.is_month_end    # Is last day of month
- .dt.weekday         # Day of week (0-6)
- .dt.strftime()      # Format datetime as string

Key concepts:
- to_datetime() parses various date formats
- .dt accessor extracts datetime components
- Datetime components return same-length Series

Task: Parse dates and extract time components for payment/income analysis.
"""

import pandas as pd


def to_datetime():
    """Convert strings to datetime objects."""
    df = pd.DataFrame({"date_str": ["2024-01-15", "2024-02-20", "2024-03-10"]})

    # Convert to datetime
    df["date"] = pd.to_datetime(df["date_str"])
    print(f"Converted:\n{df}\n")
    print(f"Dtype: {df['date'].dtype}\n")


def extract_components():
    """Extract datetime components."""
    df = pd.DataFrame(
        {"date": pd.to_datetime(["2024-01-15", "2024-02-20", "2024-03-10"])}
    )

    # Extract components
    df["year"] = df["date"].dt.year
    df["month"] = df["date"].dt.month
    df["day"] = df["date"].dt.day
    df["day_of_week"] = df["date"].dt.dayofweek
    df["day_name"] = df["date"].dt.day_name()
    df["month_name"] = df["date"].dt.month_name()
    df["quarter"] = df["date"].dt.quarter

    print(f"Extracted components:\n{df}\n")


def month_grouping():
    """Group by month for payment/income summary."""
    df = pd.DataFrame(
        {
            "date": pd.to_datetime(["2024-01-15", "2024-01-20", "2024-02-05"]),
            "amount": [100.0, 200.0, 150.0],
        }
    )

    # Extract month for grouping
    df["month"] = df["date"].dt.to_period("M")
    print(f"With month:\n{df}\n")

    # Group by month
    monthly = df.groupby("month")["amount"].sum()
    print(f"Monthly totals:\n{monthly}\n")


def format_datetime():
    """Format datetime as string."""
    df = pd.DataFrame({"date": pd.to_datetime(["2024-01-15", "2024-02-20"])})

    # Format as string
    formatted = df["date"].dt.strftime("%Y/%m/%d")
    print(f"Formatted (YYYY/MM/DD):\n{formatted}\n")

    # Month name and day
    formatted = df["date"].dt.strftime("%B %d, %Y")
    print(f"Formatted (Month DD, YYYY):\n{formatted}\n")


def date_range():
    """Create date ranges."""
    # Date range with daily frequency
    dates = pd.date_range(start="2024-01-01", periods=5, freq="D")
    print(f"Date range:\n{dates}\n")

    # Monthly frequency
    monthly = pd.date_range(start="2024-01-01", periods=3, freq="MS")
    print(f"Monthly start dates:\n{monthly}\n")


def filter_by_date():
    """Filter rows based on date."""
    df = pd.DataFrame(
        {
            "date": pd.to_datetime(["2024-01-15", "2024-02-20", "2024-03-10"]),
            "amount": [100.0, 200.0, 150.0],
        }
    )

    # Filter for January
    jan = df[df["date"].dt.month == 1]
    print(f"January records:\n{jan}\n")

    # Filter date range
    mask = (df["date"] >= "2024-02-01") & (df["date"] <= "2024-02-28")
    feb = df[mask]
    print(f"February records:\n{feb}\n")


if __name__ == "__main__":
    to_datetime()
    extract_components()
    month_grouping()
    format_datetime()
    date_range()
    filter_by_date()
