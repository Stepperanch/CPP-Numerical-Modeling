# Doxygen Documentation Setup

## Installation

Install Doxygen via Homebrew:
```bash
brew install doxygen
```

Optional (for better diagrams):
```bash
brew install graphviz
```

## Generating Documentation

### From the workspace root:
```bash
cd ~/Documents/CPP_Workspace
doxygen Doxyfile
```

This creates documentation in `docs/doxygen/html/`

### View the documentation:
```bash
open docs/doxygen/html/index.html
```

## Writing Documented Code

### Function documentation:
```cpp
/**
 * @brief Calculates projectile range using kinematic equations
 * 
 * This function computes the horizontal range of a projectile
 * launched at a given angle, assuming ideal conditions (no air
 * resistance, flat terrain).
 * 
 * @param v0 Initial velocity in meters per second
 * @param angle Launch angle in radians (0 to π/2)
 * @param g Gravitational acceleration in m/s² (default: 9.81)
 * @return Horizontal range in meters
 * 
 * @note This assumes ideal projectile motion
 * @warning Does not account for air resistance
 * 
 * @see calculateMaxHeight() for maximum height calculation
 * 
 * Example usage:
 * @code
 * double range = calculateRange(50.0, M_PI/4, 9.81);
 * std::cout << "Range: " << range << " meters" << std::endl;
 * @endcode
 */
double calculateRange(double v0, double angle, double g = 9.81) {
    return (v0 * v0 * std::sin(2 * angle)) / g;
}
```

### Class documentation:
```cpp
/**
 * @class Vector3D
 * @brief Three-dimensional vector with basic operations
 * 
 * Represents a 3D vector with x, y, z components and provides
 * common vector operations like addition, scalar multiplication,
 * and magnitude calculation.
 * 
 * @author Your Name
 * @date 2026-01-07
 */
class Vector3D {
private:
    double x, y, z;  ///< Vector components
    
public:
    /**
     * @brief Constructs a 3D vector
     * @param x X-component
     * @param y Y-component  
     * @param z Z-component
     */
    Vector3D(double x, double y, double z);
    
    /**
     * @brief Calculates vector magnitude
     * @return Length of the vector
     */
    double magnitude() const;
    
    /**
     * @brief Vector addition operator
     * @param other Vector to add
     * @return Sum of vectors
     */
    Vector3D operator+(const Vector3D& other) const;
};
```

### File documentation:
```cpp
/**
 * @file integrator.cpp
 * @brief Numerical integration methods for ODEs
 * 
 * This file implements various numerical integration methods
 * including Euler, Runge-Kutta 4th order, and adaptive step
 * size algorithms.
 * 
 * @author Your Name
 * @date 2026-01-07
 */
```

## Doxygen Commands Reference

### Basic tags:
- `@brief` - Short description (one line)
- `@param` - Parameter description
- `@return` - Return value description
- `@note` - Additional notes
- `@warning` - Important warnings
- `@see` - Cross-references

### Code examples:
- `@code ... @endcode` - Code block
- `@verbatim ... @endverbatim` - Literal text block

### Lists:
- `@li` - List item
- `-` or `*` - Bullet points in descriptions

### Math equations:
- `@f$ ... @f$` - Inline equation (LaTeX)
- `@f[ ... @f]` - Display equation

### Example with math:
```cpp
/**
 * @brief Solves quadratic equation ax² + bx + c = 0
 * 
 * Uses the quadratic formula:
 * @f[
 *     x = \frac{-b \pm \sqrt{b^2 - 4ac}}{2a}
 * @f]
 * 
 * @param a Coefficient of x²
 * @param b Coefficient of x
 * @param c Constant term
 * @return Pair of roots (x₁, x₂)
 */
```

## Customizing Appearance

### Edit Doxyfile:
```bash
# Change project name
PROJECT_NAME = "My Physics Project"

# Change output directory
OUTPUT_DIRECTORY = documentation

# Include private members
EXTRACT_PRIVATE = YES

# Generate LaTeX (for PDF)
GENERATE_LATEX = YES
```

### Regenerate after changes:
```bash
doxygen Doxyfile
```

## Integration with VS Code

### Add task to tasks.json:
```json
{
    "label": "Generate Documentation",
    "type": "shell",
    "command": "doxygen Doxyfile && open docs/doxygen/html/index.html",
    "problemMatcher": []
}
```

Run via **Cmd+Shift+P** → "Tasks: Run Task" → "Generate Documentation"

## Best Practices

1. **Document all public APIs** - Functions, classes, methods
2. **Include examples** - Show how to use complex functions
3. **Document parameters** - What they mean, valid ranges
4. **Add notes and warnings** - Important usage information
5. **Keep updated** - Regenerate docs when code changes

## Viewing Documentation

After generating:
1. Navigate to `docs/doxygen/html/`
2. Open `index.html` in browser
3. Browse by:
   - Classes
   - Files
   - Namespaces
   - Functions
   - Search

The documentation is fully searchable and cross-referenced!
