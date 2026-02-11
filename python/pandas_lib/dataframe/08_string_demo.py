"""
String Operations Demo
======================

APIs covered:
- .str.lower()        # Convert to lowercase
- .str.upper()        # Convert to uppercase
- .str.title()       # Title case
- .str.contains()    # Check if substring exists
- .str.replace()     # Replace substring
- .str.split()       # Split string
- .str.join()       # Join strings
- .str.strip()      # Remove whitespace
- .str.len()        # Length of strings
- .str.startswith() # Check prefix
- .str.endswith()   # Check suffix
- .str.extract()    # Extract using regex

Key concepts:
- .str accessor provides string methods for Series
- Methods are vectorized (apply to each element)
- Returns Series of same length

Task: Clean and manipulate string columns in payment/income data.
"""

import pandas as pd


def case_conversion():
    """Convert string case."""
    df = pd.DataFrame({"category": ["FOOD", "Transport", "ENTERTAINMENT"]})

    lower = df["category"].str.lower()
    print(f"Lowercase:\n{lower}\n")

    upper = df["category"].str.upper()
    print(f"Uppercase:\n{upper}\n")

    title = df["category"].str.title()
    print(f"Title case:\n{title}\n")


def contains_check():
    """Check if strings contain substring."""
    df = pd.DataFrame(
        {"description": ["coffee at starbucks", "bus fare", "restaurant lunch"]}
    )

    # Contains "coffee"
    has_coffee = df["description"].str.contains("coffee")
    print(f"Contains 'coffee':\n{has_coffee}\n")

    # Filter rows containing "coffee"
    coffee_rows = df[df["description"].str.contains("coffee")]
    print(f"Rows with 'coffee':\n{coffee_rows}\n")


def string_replace():
    """Replace substrings."""
    df = pd.DataFrame(
        {"category": ["food-restaurant", "food-groceries", "transport-bus"]}
    )

    # Replace hyphen with space
    replaced = df["category"].str.replace("-", " ")
    print(f"Replaced hyphen:\n{replaced}\n")


def string_split():
    """Split strings into columns."""
    df = pd.DataFrame({"category": ["food-restaurant", "transport-bus"]})

    # Split into two columns
    split = df["category"].str.split("-", expand=True)
    print(f"Split result:\n{split}\n")


def string_length():
    """Get string lengths."""
    df = pd.DataFrame({"word": ["hi", "hello", "greetings"]})

    lengths = df["word"].str.len()
    print(f"String lengths:\n{lengths}\n")


def strip_whitespace():
    """Remove leading/trailing whitespace."""
    df = pd.DataFrame({"category": ["  food  ", "transport", "  entertainment  "]})

    stripped = df["category"].str.strip()
    print(f"Stripped:\n{stripped}\n")


def startswith_endswith():
    """Check prefix and suffix."""
    df = pd.DataFrame({"category": ["food-coffee", "food-lunch", "transport-bus"]})

    starts_food = df["category"].str.startswith("food")
    print(f"Starts with 'food':\n{starts_food}\n")


def extract_strings():
    """Extract using regex."""
    df = pd.DataFrame(
        {"description": ["coffee $5.00", "lunch $12.50", "dinner $30.00"]}
    )

    # Extract amount after $
    extracted = df["description"].str.extract(r"\$(\d+\.?\d*)")
    print(f"Extracted amounts:\n{extracted}\n")


if __name__ == "__main__":
    case_conversion()
    contains_check()
    string_replace()
    string_split()
    string_length()
    strip_whitespace()
    startswith_endswith()
    extract_strings()
