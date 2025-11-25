# Documentation Deployment Guide

## Overview

The libdoip project automatically generates and publishes API documentation using Doxygen and GitHub Pages.

## How It Works

### 1. **Documentation Generation**
- Documentation is generated from Doxygen comments in header files (`.h`) and source files (`.cpp`)
- Configuration is defined in `Doxyfile` at the project root
- Includes class diagrams, call graphs, and inheritance diagrams using Graphviz

### 2. **Automated Deployment (GitHub Actions)**
- **Trigger**: Every push to `main` branch
- **Workflow**: `.github/workflows/ci.yml` → `documentation` job
- **Steps**:
  1. Installs Doxygen and Graphviz
  2. Runs `doxygen Doxyfile` to generate HTML documentation
  3. Deploys to `gh-pages` branch using `peaceiris/actions-gh-pages@v3`

### 3. **GitHub Pages Hosting**
- Documentation is automatically published at: **https://magolves.github.io/libdoip/**
- Served from the `gh-pages` branch
- Updates within minutes after push to main

## Setup Requirements

### Enable GitHub Pages

1. Go to repository **Settings** → **Pages**
2. Under **Source**, select:
   - Branch: `gh-pages`
   - Folder: `/ (root)`
3. Click **Save**

The documentation will be available at `https://<username>.github.io/<repository>/`

### Permissions

The workflow needs write permissions to create the `gh-pages` branch:
- The `ci.yml` includes `permissions: contents: write` for the documentation job
- This is automatically granted for workflows in your own repository

## Local Documentation Generation

To build documentation on your local machine:

```bash
# Install dependencies
sudo apt install doxygen graphviz

# Generate documentation
doxygen Doxyfile

# View in browser
xdg-open docs/html/index.html
```

## Documentation Best Practices

### Doxygen Comment Styles

```cpp
/**
 * @brief Short description of the function
 *
 * Longer description with more details about what
 * this function does and how to use it.
 *
 * @param paramName Description of the parameter
 * @return Description of return value
 * @throws ExceptionType When this exception is thrown
 *
 * @example
 * ByteArray arr = {0x01, 0x02, 0x03};
 * uint16_t value = arr.readU16BE(0);
 */
```

### Key Doxygen Commands

- `@brief` - Short description (one line)
- `@param` - Parameter description
- `@return` - Return value description
- `@throws` / `@exception` - Exception documentation
- `@note` - Additional notes
- `@warning` - Important warnings
- `@example` - Code examples
- `@see` - Related items
- `@deprecated` - Mark as deprecated

### File Header Example

```cpp
/**
 * @file ByteArray.h
 * @brief Byte array utilities for network protocol handling
 *
 * This file provides a ByteArray type with methods for reading
 * and writing multi-byte integers in big-endian format.
 */
```

## Maintenance

### Updating Documentation

Documentation updates automatically when you:
1. Add/modify Doxygen comments in code
2. Push changes to `main` branch
3. GitHub Actions builds and deploys new version

### Configuration Changes

To modify documentation generation:
- Edit `Doxyfile` for Doxygen settings
- Edit `.github/workflows/ci.yml` for deployment settings

### Troubleshooting

**Documentation not updating?**
1. Check GitHub Actions: Go to **Actions** tab
2. Look for failed `documentation` job
3. Review build logs for errors

**404 on documentation page?**
1. Verify GitHub Pages is enabled in Settings
2. Check that `gh-pages` branch exists
3. Wait a few minutes after first push

**Missing diagrams?**
- Ensure `HAVE_DOT = YES` in Doxyfile
- Graphviz must be installed in CI (already configured)

## Alternative: GitHub Wiki

While we use GitHub Pages, you can also manually upload documentation to Wiki:

```bash
# Clone the wiki repository
git clone https://github.com/Magolves/libdoip.wiki.git

# Copy generated documentation
cp -r docs/html/* libdoip.wiki/

# Commit and push
cd libdoip.wiki
git add .
git commit -m "Update documentation"
git push
```

**However**, GitHub Pages is recommended because:
- ✅ Fully automated
- ✅ Preserves full HTML structure and styling
- ✅ Better for large API documentation
- ✅ Supports search functionality
- ✅ Maintains versioning through git

## Resources

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [GitHub Pages Documentation](https://docs.github.com/en/pages)
- [Doxygen Special Commands](https://www.doxygen.nl/manual/commands.html)
- [peaceiris/actions-gh-pages](https://github.com/peaceiris/actions-gh-pages)
