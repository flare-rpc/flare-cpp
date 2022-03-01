// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#include <pthread.h>
#include "flare/rpc/builtin/sorttable_js.h"


namespace flare::rpc {

static pthread_once_t s_sorttable_buf_once = PTHREAD_ONCE_INIT; 
static flare::cord_buf* s_sorttable_buf = NULL;
static void InitSortTableBuf() {
    s_sorttable_buf = new flare::cord_buf;
    s_sorttable_buf->append(sorttable_js());
}
const flare::cord_buf& sorttable_js_iobuf() {
    pthread_once(&s_sorttable_buf_once, InitSortTableBuf);
    return *s_sorttable_buf;
}

/*
  SortTable
  version 2
  7th April 2007
  Stuart Langridge, http://www.kryogenix.org/code/browser/sorttable/

  Instructions:
  Download this file
  Add <script src=\"sorttable.js\"></script> to your HTML
  Add class=\"sortable\" to any table you'd like to make sortable
  Click on the headers to sort

  Thanks to many, many people for contributions and suggestions.
  Licenced as X11: http://www.kryogenix.org/code/browser/licence.html
  This basically means: do what you want with it.
*/

const char* sorttable_js() {
    return "var stIsIE = /*@cc_on!@*/false;\n"
"sorttable = {\n"
"  init: function() {\n"
"    // quit if this function has already been called\n"
"    if (arguments.callee.done) return;\n"
"    // flag this function so we don't do the same thing twice\n"
"    arguments.callee.done = true;\n"
"    // kill the timer\n"
"    if (_timer) clearInterval(_timer);\n"
"\n"
"    if (!document.createElement || !document.getElementsByTagName) return;\n"
"\n"
"    sorttable.DATE_RE = /^(\\d\\d?)[\\/\\.-](\\d\\d?)[\\/\\.-]((\\d\\d)?\\d\\d)$/;\n"
"\n"
"    forEach(document.getElementsByTagName('table'), function(table) {\n"
"      if (table.className.search(/\\bsortable\\b/) != -1) {\n"
"        sorttable.makeSortable(table);\n"
"      }\n"
"    });\n"
"\n"
"  },\n"
"\n"
"  makeSortable: function(table) {\n"
"    if (table.getElementsByTagName('thead').length == 0) {\n"
"      // table doesn't have a tHead. Since it should have, create one and\n"
"      // put the first table row in it.\n"
"      the = document.createElement('thead');\n"
"      the.appendChild(table.rows[0]);\n"
"      table.insertBefore(the,table.firstChild);\n"
"    }\n"
"    // Safari doesn't support table.tHead, sigh\n"
"    if (table.tHead == null) table.tHead = table.getElementsByTagName('thead')[0];\n"
"\n"
"    if (table.tHead.rows.length != 1) return; // can't cope with two header rows\n"
"\n"
"    // Sorttable v1 put rows with a class of \"sortbottom\" at the bottom (as\n"
"    // \"total\" rows, for example). This is B&R, since what you're supposed\n"
"    // to do is put them in a tfoot. So, if there are sortbottom rows,\n"
"    // for backwards compatibility, move them to tfoot (creating it if needed).\n"
"    sortbottomrows = [];\n"
"    for (var i=0; i<table.rows.length; i++) {\n"
"      if (table.rows[i].className.search(/\\bsortbottom\\b/) != -1) {\n"
"        sortbottomrows[sortbottomrows.length] = table.rows[i];\n"
"      }\n"
"    }\n"
"    if (sortbottomrows) {\n"
"      if (table.tFoot == null) {\n"
"        // table doesn't have a tfoot. Create one.\n"
"        tfo = document.createElement('tfoot');\n"
"        table.appendChild(tfo);\n"
"      }\n"
"      for (var i=0; i<sortbottomrows.length; i++) {\n"
"        tfo.appendChild(sortbottomrows[i]);\n"
"      }\n"
"      delete sortbottomrows;\n"
"    }\n"
"\n"
"    // work through each column and calculate its type\n"
"    headrow = table.tHead.rows[0].cells;\n"
"    for (var i=0; i<headrow.length; i++) {\n"
"      // manually override the type with a sorttable_type attribute\n"
"      if (!headrow[i].className.match(/\\bsorttable_nosort\\b/)) { // skip this col\n"
"        mtch = headrow[i].className.match(/\\bsorttable_([a-z0-9]+)\\b/);\n"
"        if (mtch) { override = mtch[1]; }\n"
"          if (mtch && typeof sorttable[\"sort_\"+override] == 'function') {\n"
"            headrow[i].sorttable_sortfunction = sorttable[\"sort_\"+override];\n"
"          } else {\n"
"            headrow[i].sorttable_sortfunction = sorttable.guessType(table,i);\n"
"          }\n"
"          // make it clickable to sort\n"
"          headrow[i].sorttable_columnindex = i;\n"
"          headrow[i].sorttable_tbody = table.tBodies[0];\n"
"          dean_addEvent(headrow[i],\"click\", sorttable.innerSortFunction = function(e) {\n"
"\n"
"          if (this.className.search(/\\bsorttable_sorted\\b/) != -1) {\n"
"            // if we're already sorted by this column, just\n"
"            // reverse the table, which is quicker\n"
"            sorttable.reverse(this.sorttable_tbody);\n"
"            this.className = this.className.replace('sorttable_sorted',\n"
"                                                    'sorttable_sorted_reverse');\n"
"            this.removeChild(document.getElementById('sorttable_sortfwdind'));\n"
"            sortrevind = document.createElement('span');\n"
"            sortrevind.id = \"sorttable_sortrevind\";\n"
"            sortrevind.innerHTML = stIsIE ? '&nbsp<font face=\"webdings\">5</font>' :\n"
"'&nbsp;&#x25B4;';\n"
"            this.appendChild(sortrevind);\n"
"            return;\n"
"          }\n"
"          if (this.className.search(/\\bsorttable_sorted_reverse\\b/) != -1) {\n"
"            // if we're already sorted by this column in reverse, just\n"
"            // re-reverse the table, which is quicker\n"
"            sorttable.reverse(this.sorttable_tbody);\n"
"            this.className = this.className.replace('sorttable_sorted_reverse',\n"
"                                                    'sorttable_sorted');\n"
"            this.removeChild(document.getElementById('sorttable_sortrevind'));\n"
"            sortfwdind = document.createElement('span');\n"
"            sortfwdind.id = \"sorttable_sortfwdind\";\n"
"            sortfwdind.innerHTML = stIsIE ? '&nbsp<font face=\"webdings\">6</font>' :\n"
"'&nbsp;&#x25BE;';\n"
"            this.appendChild(sortfwdind);\n"
"            return;\n"
"          }\n"
"\n"
"          // remove sorttable_sorted classes\n"
"          theadrow = this.parentNode;\n"
"          forEach(theadrow.childNodes, function(cell) {\n"
"            if (cell.nodeType == 1) { // an element\n"
"              cell.className = cell.className.replace('sorttable_sorted_reverse','');\n"
"              cell.className = cell.className.replace('sorttable_sorted','');\n"
"            }\n"
"          });\n"
"          sortfwdind = document.getElementById('sorttable_sortfwdind');\n"
"          if (sortfwdind) { sortfwdind.parentNode.removeChild(sortfwdind); }\n"
"          sortrevind = document.getElementById('sorttable_sortrevind');\n"
"          if (sortrevind) { sortrevind.parentNode.removeChild(sortrevind); }\n"
"\n"
"          this.className += ' sorttable_sorted';\n"
"          sortfwdind = document.createElement('span');\n"
"          sortfwdind.id = \"sorttable_sortfwdind\";\n"
"          sortfwdind.innerHTML = stIsIE ? '&nbsp<font face=\"webdings\">6</font>' : '&nbsp;&#x25BE;';\n"
"          this.appendChild(sortfwdind);\n"
"\n"
"            // build an array to sort. This is a Schwartzian transform thing,\n"
"            // i.e., we \"decorate\" each row with the actual sort key,\n"
"            // sort based on the sort keys, and then put the rows back in order\n"
"            // which is a lot faster because you only do getInnerText once per row\n"
"            row_array = [];\n"
"            col = this.sorttable_columnindex;\n"
"            rows = this.sorttable_tbody.rows;\n"
"            for (var j=0; j<rows.length; j++) {\n"
"              row_array[row_array.length] = [sorttable.getInnerText(rows[j].cells[col]), rows[j]];\n"
"            }\n"
"            //sorttable.shaker_sort(row_array, this.sorttable_sortfunction);\n"
"            row_array.sort(this.sorttable_sortfunction);\n"
"\n"
"            tb = this.sorttable_tbody;\n"
"            for (var j=0; j<row_array.length; j++) {\n"
"              tb.appendChild(row_array[j][1]);\n"
"            }\n"
"\n"
"            delete row_array;\n"
"          });\n"
"        }\n"
"    }\n"
"  },\n"
"\n"
"  guessType: function(table, column) {\n"
"    // guess the type of a column based on its first non-blank row\n"
"    sortfn = sorttable.sort_alpha;\n"
"    for (var i=0; i<table.tBodies[0].rows.length; i++) {\n"
"      text = sorttable.getInnerText(table.tBodies[0].rows[i].cells[column]);\n"
"      if (text != '') {\n"
"        if (text.match(/^-?[$]?[\\d,.]+%?$/)) {\n"
"          return sorttable.sort_numeric;\n"
"        }\n"
"        // check for a date: dd/mm/yyyy or dd/mm/yy\n"
"        // can have / or . or - as separator\n"
"        // can be mm/dd as well\n"
"        possdate = text.match(sorttable.DATE_RE)\n"
"        if (possdate) {\n"
"          // looks like a date\n"
"          first = parseInt(possdate[1]);\n"
"          second = parseInt(possdate[2]);\n"
"          if (first > 12) {\n"
"            // definitely dd/mm\n"
"            return sorttable.sort_ddmm;\n"
"          } else if (second > 12) {\n"
"            return sorttable.sort_mmdd;\n"
"          } else {\n"
"            // looks like a date, but we can't tell which, so assume\n"
"            // that it's dd/mm (English imperialism!) and keep looking\n"
"            sortfn = sorttable.sort_ddmm;\n"
"          }\n"
"        }\n"
"      }\n"
"    }\n"
"    return sortfn;\n"
"  },\n"
"\n"
"  getInnerText: function(node) {\n"
"    // gets the text we want to use for sorting for a cell.\n"
"    // strips leading and trailing whitespace.\n"
"    // this is *not* a generic getInnerText function; it's special to sorttable.\n"
"    // for example, you can override the cell text with a customkey attribute.\n"
"    // it also gets .value for <input> fields.\n"
"\n"
"    if (!node) return \"\";\n"
"\n"
"    hasInputs = (typeof node.getElementsByTagName == 'function') &&\n"
"                 node.getElementsByTagName('input').length;\n"
"\n"
"    if (node.getAttribute(\"sorttable_customkey\") != null) {\n"
"      return node.getAttribute(\"sorttable_customkey\");\n"
"    }\n"
"    else if (typeof node.textContent != 'undefined' && !hasInputs) {\n"
"      return node.textContent.replace(/^\\s+|\\s+$/g, '');\n"
"    }\n"
"    else if (typeof node.innerText != 'undefined' && !hasInputs) {\n"
"      return node.innerText.replace(/^\\s+|\\s+$/g, '');\n"
"    }\n"
"    else if (typeof node.text != 'undefined' && !hasInputs) {\n"
"      return node.text.replace(/^\\s+|\\s+$/g, '');\n"
"    }\n"
"    else {\n"
"      switch (node.nodeType) {\n"
"        case 3:\n"
"          if (node.nodeName.toLowerCase() == 'input') {\n"
"            return node.value.replace(/^\\s+|\\s+$/g, '');\n"
"          }\n"
"        case 4:\n"
"          return node.nodeValue.replace(/^\\s+|\\s+$/g, '');\n"
"          break;\n"
"        case 1:\n"
"        case 11:\n"
"          var innerText = '';\n"
"          for (var i = 0; i < node.childNodes.length; i++) {\n"
"            innerText += sorttable.getInnerText(node.childNodes[i]);\n"
"          }\n"
"          return innerText.replace(/^\\s+|\\s+$/g, '');\n"
"          break;\n"
"        default:\n"
"          return '';\n"
"      }\n"
"    }\n"
"  },\n"
"\n"
"  reverse: function(tbody) {\n"
"    // reverse the rows in a tbody\n"
"    newrows = [];\n"
"    for (var i=0; i<tbody.rows.length; i++) {\n"
"      newrows[newrows.length] = tbody.rows[i];\n"
"    }\n"
"    for (var i=newrows.length-1; i>=0; i--) {\n"
"       tbody.appendChild(newrows[i]);\n"
"    }\n"
"    delete newrows;\n"
"  },\n"
"\n"
"  sort_numeric: function(a,b) {\n"
"    aa = parseFloat(a[0].replace(/[^0-9.-]/g,''));\n"
"    if (isNaN(aa)) aa = 0;\n"
"    bb = parseFloat(b[0].replace(/[^0-9.-]/g,''));\n"
"    if (isNaN(bb)) bb = 0;\n"
"    return aa-bb;\n"
"  },\n"
"  sort_alpha: function(a,b) {\n"
"    if (a[0]==b[0]) return 0;\n"
"    if (a[0]<b[0]) return -1;\n"
"    return 1;\n"
"  },\n"
"  sort_ddmm: function(a,b) {\n"
"    mtch = a[0].match(sorttable.DATE_RE);\n"
"    y = mtch[3]; m = mtch[2]; d = mtch[1];\n"
"    if (m.length == 1) m = '0'+m;\n"
"    if (d.length == 1) d = '0'+d;\n"
"    dt1 = y+m+d;\n"
"    mtch = b[0].match(sorttable.DATE_RE);\n"
"    y = mtch[3]; m = mtch[2]; d = mtch[1];\n"
"    if (m.length == 1) m = '0'+m;\n"
"    if (d.length == 1) d = '0'+d;\n"
"    dt2 = y+m+d;\n"
"    if (dt1==dt2) return 0;\n"
"    if (dt1<dt2) return -1;\n"
"    return 1;\n"
"  },\n"
"  sort_mmdd: function(a,b) {\n"
"    mtch = a[0].match(sorttable.DATE_RE);\n"
"    y = mtch[3]; d = mtch[2]; m = mtch[1];\n"
"    if (m.length == 1) m = '0'+m;\n"
"    if (d.length == 1) d = '0'+d;\n"
"    dt1 = y+m+d;\n"
"    mtch = b[0].match(sorttable.DATE_RE);\n"
"    y = mtch[3]; d = mtch[2]; m = mtch[1];\n"
"    if (m.length == 1) m = '0'+m;\n"
"    if (d.length == 1) d = '0'+d;\n"
"    dt2 = y+m+d;\n"
"    if (dt1==dt2) return 0;\n"
"    if (dt1<dt2) return -1;\n"
"    return 1;\n"
"  },\n"
"\n"
"  shaker_sort: function(list, comp_func) {\n"
"    // A stable sort function to allow multi-level sorting of data\n"
"    // see: http://en.wikipedia.org/wiki/Cocktail_sort\n"
"    // thanks to Joseph Nahmias\n"
"    var b = 0;\n"
"    var t = list.length - 1;\n"
"    var swap = true;\n"
"\n"
"    while(swap) {\n"
"        swap = false;\n"
"        for(var i = b; i < t; ++i) {\n"
"            if ( comp_func(list[i], list[i+1]) > 0 ) {\n"
"                var q = list[i]; list[i] = list[i+1]; list[i+1] = q;\n"
"                swap = true;\n"
"            }\n"
"        } // for\n"
"        t--;\n"
"\n"
"        if (!swap) break;\n"
"\n"
"        for(var i = t; i > b; --i) {\n"
"            if ( comp_func(list[i], list[i-1]) < 0 ) {\n"
"                var q = list[i]; list[i] = list[i-1]; list[i-1] = q;\n"
"                swap = true;\n"
"            }\n"
"        } // for\n"
"        b++;\n"
"\n"
"    } // while(swap)\n"
"  }\n"
"}\n"
"\n"
"/* ******************************************************************\n"
"   Supporting functions: bundled here to avoid depending on a library\n"
"   ****************************************************************** */\n"
"\n"
"// Dean Edwards/Matthias Miller/John Resig\n"
"\n"
"/* for Mozilla/Opera9 */\n"
"if (document.addEventListener) {\n"
"    document.addEventListener(\"DOMContentLoaded\", sorttable.init, false);\n"
"}\n"
"\n"
"/* for Internet Explorer */\n"
"/*@cc_on @*/\n"
"/*@if (@_win32)\n"
"    document.write(\"<script id=__ie_onload defer src=javascript:void(0)><\\/script>\");\n"
"    var script = document.getElementById(\"__ie_onload\");\n"
"    script.onreadystatechange = function() {\n"
"        if (this.readyState == \"complete\") {\n"
"            sorttable.init(); // call the onload handler\n"
"        }\n"
"    };\n"
"/*@end @*/\n"
"\n"
"/* for Safari */\n"
"if (/WebKit/i.test(navigator.userAgent)) { // sniff\n"
"    var _timer = setInterval(function() {\n"
"        if (/loaded|complete/.test(document.readyState)) {\n"
"            sorttable.init(); // call the onload handler\n"
"        }\n"
"    }, 10);\n"
"}\n"
"\n"
"/* for other browsers */\n"
"window.onload = sorttable.init;\n"
"\n"
"// written by Dean Edwards, 2005\n"
"// with input from Tino Zijdel, Matthias Miller, Diego Perini\n"
"\n"
"// http://dean.edwards.name/weblog/2005/10/add-event/\n"
"\n"
"function dean_addEvent(element, type, handler) {\n"
"    if (element.addEventListener) {\n"
"        element.addEventListener(type, handler, false);\n"
"    } else {\n"
"        // assign each event handler a unique ID\n"
"        if (!handler.$$guid) handler.$$guid = dean_addEvent.guid++;\n"
"        // create a hash table of event types for the element\n"
"        if (!element.events) element.events = {};\n"
"        // create a hash table of event handlers for each element/event pair\n"
"        var handlers = element.events[type];\n"
"        if (!handlers) {\n"
"            handlers = element.events[type] = {};\n"
"            // store the existing event handler (if there is one)\n"
"            if (element[\"on\" + type]) {\n"
"                handlers[0] = element[\"on\" + type];\n"
"            }\n"
"        }\n"
"        // store the event handler in the hash table\n"
"        handlers[handler.$$guid] = handler;\n"
"        // assign a global event handler to do all the work\n"
"        element[\"on\" + type] = handleEvent;\n"
"    }\n"
"};\n"
"// a counter used to create unique IDs\n"
"dean_addEvent.guid = 1;\n"
"\n"
"function removeEvent(element, type, handler) {\n"
"    if (element.removeEventListener) {\n"
"        element.removeEventListener(type, handler, false);\n"
"    } else {\n"
"        // delete the event handler from the hash table\n"
"        if (element.events && element.events[type]) {\n"
"            delete element.events[type][handler.$$guid];\n"
"        }\n"
"    }\n"
"};\n"
"\n"
"function handleEvent(event) {\n"
"    var returnValue = true;\n"
"    // grab the event object (IE uses a global event object)\n"
"    event = event || fixEvent(((this.ownerDocument || this.document || this).parentWindow ||\n"
"window).event);\n"
"    // get a reference to the hash table of event handlers\n"
"    var handlers = this.events[event.type];\n"
"    // execute each event handler\n"
"    for (var i in handlers) {\n"
"        this.$$handleEvent = handlers[i];\n"
"        if (this.$$handleEvent(event) === false) {\n"
"            returnValue = false;\n"
"        }\n"
"    }\n"
"    return returnValue;\n"
"};\n"
"\n"
"function fixEvent(event) {\n"
"    // add W3C standard event methods\n"
"    event.preventDefault = fixEvent.preventDefault;\n"
"    event.stopPropagation = fixEvent.stopPropagation;\n"
"    return event;\n"
"};\n"
"fixEvent.preventDefault = function() {\n"
"    this.returnValue = false;\n"
"};\n"
"fixEvent.stopPropagation = function() {\n"
"  this.cancelBubble = true;\n"
"}\n"
"\n"
"// Dean's forEach: http://dean.edwards.name/flare/forEach.js\n"
"/*\n"
"    forEach, version 1.0\n"
"    Copyright 2006, Dean Edwards\n"
"    License: http://www.opensource.org/licenses/mit-license.php\n"
"*/\n"
"\n"
"// array-like enumeration\n"
"if (!Array.forEach) { // mozilla already supports this\n"
"    Array.forEach = function(array, block, context) {\n"
"        for (var i = 0; i < array.length; i++) {\n"
"            block.call(context, array[i], i, array);\n"
"        }\n"
"    };\n"
"}\n"
"\n"
"// generic enumeration\n"
"Function.prototype.forEach = function(object, block, context) {\n"
"    for (var key in object) {\n"
"        if (typeof this.prototype[key] == \"undefined\") {\n"
"            block.call(context, object[key], key, object);\n"
"        }\n"
"    }\n"
"};\n"
"\n"
"// character enumeration\n"
"String.forEach = function(string, block, context) {\n"
"    Array.forEach(string.split(\"\"), function(chr, index) {\n"
"        block.call(context, chr, index, string);\n"
"    });\n"
"};\n"
"\n"
"// globally resolve forEach enumeration\n"
"var forEach = function(object, block, context) {\n"
"    if (object) {\n"
"        var resolve = Object; // default\n"
"        if (object instanceof Function) {\n"
"            // functions have a \"length\" property\n"
"            resolve = Function;\n"
"        } else if (object.forEach instanceof Function) {\n"
"            // the object implements a custom forEach method so use that\n"
"            object.forEach(block, context);\n"
"            return;\n"
"        } else if (typeof object == \"string\") {\n"
"            // the object is a string\n"
"            resolve = String;\n"
"        } else if (typeof object.length == \"number\") {\n"
"            // the object is array-like\n"
"            resolve = Array;\n"
"        }\n"
"        resolve.forEach(object, block, context);\n"
"    }\n"
"};";
}

} // namespace flare::rpc
