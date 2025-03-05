import datetime

current_year = datetime.datetime.now().year

# General information about the project.
project = 'HeadTracker_ESP32'
copyright = u'2025 - {} NineDayCC'.format(current_year)
author = 'NineDayCC'
release = 'v0.1.1'

extensions = [
    'myst_parser',
    "sphinx_design",
    'sphinx_copybutton',
    'sphinx_togglebutton',
    "sphinx_tippy",
]

myst_enable_extensions = [
    "amsmath",
    "attrs_inline",
    "colon_fence",
    "deflist",
    "dollarmath",
    "fieldlist",
    "html_admonition",
    "html_image",
    # "linkify",
    "replacements",
    "smartquotes",
    "strikethrough",
    "substitution",
    "tasklist",
]

languages = {
    'zh_CN': '中文',
    'en': 'English'
}

html_theme = 'sphinx_book_theme'
html_last_updated_fmt = ""
html_title = "HeadTracker_ESP32"
html_static_path = ['../_static']
html_css_files = ["local.css"]

html_theme_options = {
    "home_page_in_toc": True,
    "repository_url": "https://github.com/NineDayCC/HeadTracker_ESP32",
    "use_source_button": True,
    "repository_branch": "master",
    "use_edit_page_button": True,
    "use_issues_button": True,
    "show_navbar_depth": 2,
    "show_toc_level": 2,
}

tippy_skip_anchor_classes = ("headerlink", "sd-stretched-link", "sd-rounded-pill")
tippy_anchor_parent_selector = "article.bd-article"
tippy_rtd_urls = [
    "https://www.sphinx-doc.org/en/master",
]