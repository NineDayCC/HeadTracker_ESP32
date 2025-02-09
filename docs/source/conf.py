# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'HeadTracker_ESP32'
copyright = '2025, NineDayCC'
author = 'NineDayCC'
release = 'v0.1.1'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'myst_parser'  # 启用 Markdown 支持
]

templates_path = ['_templates']
exclude_patterns = []

# 启用 gettext 支持
locale_dirs = ['locale/']  # 翻译文件存放目录
gettext_compact = False    # 保留完整的目录结构

# 配置可用语言
language = 'zh_CN'  # 默认语言
languages = {
    'zh_CN': '中文',
    'en': 'English'
}

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
