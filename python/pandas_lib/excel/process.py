"""
Excel Payment-Income Processor
==============================

Process personal payment/income records from Excel and generate monthly summaries.

Workflow:
1. Load payments and income sheets from Excel
2. Clean and standardize data
3. Aggregate by month
4. Merge payments and income
5. Output monthly summary
"""

import argparse
import os
import pandas as pd


def load_data(input_path):
    """Load payments and income sheets from Excel."""
    payments = pd.read_excel(input_path, sheet_name="payments")
    income = pd.read_excel(input_path, sheet_name="income")
    return payments, income


def clean_data(df, name):
    """Clean and standardize DataFrame."""
    # Ensure date column exists
    if "date" not in df.columns:
        raise ValueError(f"{name} sheet missing 'date' column")

    # Parse dates
    df["date"] = pd.to_datetime(df["date"])

    # Ensure amount is numeric
    df["amount"] = pd.to_numeric(df["amount"], errors="coerce")

    # Fill missing values
    df["amount"] = df["amount"].fillna(0)
    df["description"] = df["description"].fillna("")

    return df


def aggregate_by_month(df):
    """Aggregate amounts by month."""
    # Extract month period
    df["month"] = df["date"].dt.to_period("M")

    # Group by month and sum
    monthly = df.groupby("month")["amount"].sum().reset_index()
    monthly.columns = ["month", "amount"]

    return monthly


def merge_summaries(payments_monthly, income_monthly):
    """Merge payments and income summaries."""
    # Merge on month (outer join to include all months)
    merged = pd.merge(
        payments_monthly,
        income_monthly,
        on="month",
        how="outer",
        suffixes=("_payment", "_income"),
    )

    # Fill missing with 0
    merged["amount_payment"] = merged["amount_payment"].fillna(0)
    merged["amount_income"] = merged["amount_income"].fillna(0)

    # Calculate net
    merged["net"] = merged["amount_income"] - merged["amount_payment"]

    # Rename columns for clarity
    merged.columns = ["month", "total_payment", "total_income", "net"]

    # Sort by month
    merged = merged.sort_values("month")

    return merged


def output_summary(df, output_path):
    """Write summary to Excel."""
    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    # Write to Excel
    df.to_excel(output_path, sheet_name="monthly_summary", index=False)
    print(f"Output written to: {output_path}")


def create_sample_file(output_path):
    """Create sample Excel file with test data."""
    # Sample payments data
    payments = pd.DataFrame(
        {
            "date": [
                "2024-01-15",
                "2024-01-20",
                "2024-01-25",
                "2024-02-05",
                "2024-02-10",
                "2024-02-15",
                "2024-02-20",
            ],
            "amount": [50.0, 100.0, 30.0, 80.0, 120.0, 45.0, 200.0],
            "description": [
                "coffee",
                "lunch",
                "groceries",
                "bus fare",
                "dinner",
                "snack",
                "gift",
            ],
        }
    )

    # Sample income data
    income = pd.DataFrame(
        {
            "date": ["2024-01-01", "2024-02-01", "2024-01-10", "2024-02-15"],
            "amount": [5000.0, 5200.0, 500.0, 300.0],
            "description": ["salary", "salary", "freelance", "bonus"],
        }
    )

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    # Write to Excel with two sheets
    with pd.ExcelWriter(output_path, engine="openpyxl") as writer:
        payments.to_excel(writer, sheet_name="payments", index=False)
        income.to_excel(writer, sheet_name="income", index=False)

    print(f"Sample file created: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Process payment/income Excel file")
    parser.add_argument(
        "--create-sample", action="store_true", help="Create sample input file"
    )
    parser.add_argument(
        "--input", default="input/records.xlsx", help="Input Excel file path"
    )
    parser.add_argument(
        "--output", default="output/monthly.xlsx", help="Output Excel file path"
    )

    args = parser.parse_args()

    if args.create_sample:
        create_sample_file(args.input)
        return

    # Load data
    print(f"Loading data from: {args.input}")
    payments, income = load_data(args.input)

    # Clean data
    print("Cleaning data...")
    payments = clean_data(payments, "payments")
    income = clean_data(income, "income")

    # Aggregate by month
    print("Aggregating by month...")
    payments_monthly = aggregate_by_month(payments)
    income_monthly = aggregate_by_month(income)

    # Merge summaries
    print("Merging payments and income...")
    summary = merge_summaries(payments_monthly, income_monthly)

    # Display summary
    print("\nMonthly Summary:")
    print(summary.to_string(index=False))

    # Output
    output_summary(summary, args.output)


if __name__ == "__main__":
    main()
