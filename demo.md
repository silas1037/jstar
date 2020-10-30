---
layout: default
title: Demo
nav_order: 2
description: "J* demo"
permalink: /demo
last_modified_date: 2020-04-27T17:54:08+0000
---

<script src="{{ 'assets/js/codejar.js' | absolute_url }}"></script>
<script src="{{ 'assets/js/linenumbers.js' | absolute_url }}"></script>
<script src="{{ 'assets/js/jstar-demo.js' | absolute_url }}"></script>

Examples
{: .text-delta }
<button type="button" id="hello-world" class="btn btn-blue">Hello World</button>
<button type="button" id="loop" class="btn btn-blue">Loop</button>
<button type="button" id="quick-sort" class="btn btn-blue">Quick Sort</button>
<button type="button" id="regex" class="btn btn-blue">Regex</button>

---

Type your code here
{: .text-delta }
<div id="input" class="editor language-jstar">print('Hello, World!')</div>

<p id="error-label" style="float: right; margin-top: 2em" hidden></p>
<button type="button" id="run" class="btn btn-green">&#9654; Run</button>
<pre class="editor" style="overflow-x: scroll;"><div id="output">[...]</div></pre>