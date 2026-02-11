"""
DataFrame Creation Demo
=======================

APIs covered:
- pd.DataFrame()       # Create DataFrame from dict, list, or other structures
- pd.Series()         # Create 1D labeled array
- pd.DataFrame.from_dict()  # Create from dict with specific orientation
- pd.DataFrame.from_records()  # Create from structured array or dict

Task: Create DataFrames and Series using various methods with payment/income data.
"""

import pandas as pd


def create_from_dict():
    """Create DataFrame from dictionary (most common method)."""
    # Each key becomes a column: the dimension / length of each list must match
    payments = pd.DataFrame(
        {
            "date": ["2024-01-15", "2024-01-20", "2024-02-05"],
            "amount": [100.0, 200.0, 50.0],
            "category": ["food", "transport", "food"],
        }
    )
    print(payments)
    # scalar and list may be mixed together and broadcasted
    payments = pd.DataFrame(
        {
            "date": "2026-02-11"
            "amount": [100.0, 200.0, 50.0],
            "category": "food",
        }
    )
    print(payments)


def create_from_list():
    """Create DataFrame from list of lists (no column names)."""
    # Must specify columns explicitly
    data = [
        # Each internal list represents a row
        ["2024-01-15", 100.0, "food"],
        ["2024-01-20", 200.0, "transport"],
        ["2024-02-05", 50.0, "food"],
    ]
    df = pd.DataFrame(data, columns=["date", "amount", "category"])
    print(df)
    print()


def create_with_index():
    """Create DataFrame with custom index."""
    df = pd.DataFrame(
        {"date": ["2024-01-15", "2024-01-20"], "amount": [100.0, 200.0]},
        index=["payment1", "payment2"],
    )
    print(df)
    print()


def create_series():
    """Create Series (1D array with index)."""
    # Series is a single column with index
    amounts = pd.Series([100.0, 200.0, 50.0], name="amount")
    print(amounts)
    print()

    # Series with custom index
    labeled_amounts = pd.Series([100.0, 200.0], index=["coffee", "lunch"])
    print(labeled_amounts)
    print()


def create_from_records():
    """Create DataFrame from list of dictionaries (records format)."""
    # Each dict is a row, keys are columns
    records = [
        {"date": "2024-01-15", "amount": 100.0, "category": "food"},
        {"date": "2024-01-20", "amount": 200.0, "category": "transport"},
    ]
    df = pd.DataFrame(records)
    print(df)
    print()


if __name__ == "__main__":
    create_from_dict()
    create_from_list()
    create_with_index()
    create_series()
    create_from_records()
