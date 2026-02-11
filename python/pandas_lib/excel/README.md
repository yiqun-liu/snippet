# Excel Processing Demo

Process personal payment/income records from Excel to generate monthly summaries.

## What This Project Does

| Stage | Description |
|-------|-------------|
| **Input** | Excel file with two sheets: `payments` and `income` |
| **Process** | Clean, aggregate by month |
| **Output** | Monthly payment-income summary Excel file |

## Input Data Format

### payments Sheet
| Column | Type | Description |
|--------|------|-------------|
| date | string | Transaction date (YYYY-MM-DD) |
| amount | float | Payment amount (positive) |
| description | string | Transaction description |

### income Sheet
| Column | Type | Description |
|--------|------|-------------|
| date | string | Transaction date (YYYY-MM-DD) |
| amount | float | Income amount (positive) |
| description | string | Income description |

## Output Format

### monthly_summary Sheet
| Column | Type | Description |
|--------|------|-------------|
| month | string | Year-Month (YYYY-MM) |
| total_income | float | Sum of income for month |
| total_payment | float | Sum of payments for month |
| net | float | Income - Payment |

## Key Pandas APIs Used

### Excel I/O
| API | Description |
|-----|-------------|
| `pd.read_excel()` | Read Excel file |
| `to_excel()` | Write DataFrame to Excel |

### Data Processing
| API | Description |
|-----|-------------|
| `pd.to_datetime()` | Parse date strings |
| `.dt.to_period('M')` | Extract month period |
| `.groupby()` | Group by month |
| `.agg()` | Aggregate with sum |
| `pd.merge()` | Merge payments and income |

## Running the Project

```bash
# Create sample input file
python process.py --create-sample

# Process the Excel file
python process.py --input input/records.xlsx --output output/monthly.xlsx
```

## File Structure

```
excel/
├── README.md
├── process.py          # Main processing script
├── input/
│   └── records.xlsx    # Input Excel file
└── output/
    └── monthly.xlsx    # Output summary
```
