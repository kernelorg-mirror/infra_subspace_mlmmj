<?php

/* mlmmj/php-admin:
 * Copyright (C) 2023 Franky Van Liedekerke <franky at e-dynamics dot be>
 * Copyright (C) 2004 Christoph Thiel <ct at kki dot org>
 *
 * mlmmj/php-perl:
 * Copyright (C) 2004 Morten K. Poulsen <morten at afdelingp.dk>
 * Copyright (C) 2004 Christian Laursen <christian@pil.dk>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

require(dirname(dirname(__FILE__))."/conf/config.php");
require(dirname(__FILE__)."/class.rFastTemplate.php");

$tpl = new rFastTemplate($templatedir);

$tpl->define(array("main" => "index.html"));

$lists = "";

# use scandir to have alphabetical order
foreach (scandir($topdir) as $mlmmj_list) {
    if (substr($mlmmj_list,0,1) != '.' && file_exists("$topdir/$mlmmj_list/control")) {
	$admin_userfile = "$topdir/$mlmmj_list/control/admin_users";
	$subs_userfile = "$topdir/$mlmmj_list/control/subsadmin_users";
	if (!is_file($subs_userfile)) {
		$subs_userfile = "$topdir/$mlmmj_list/control/admin_users";
	}

	if (isset($_SERVER['PHP_AUTH_USER']) && is_file($admin_userfile)) {
		// read users into array
		$admin_users = array_map('trim',file($admin_userfile));
		// remove empty values from array
		$admin_users = array_filter($admin_users);
		if (in_array($_SERVER['PHP_AUTH_USER'],$admin_users)) {
			$print_config=1;
		} else {
			$print_config=0;
		}
	} else {
		$print_config=1;
	}
	if (isset($_SERVER['PHP_AUTH_USER']) && is_file($subs_userfile)) {
		// read users into array
		$subs_users = array_map('trim',file($subs_userfile));
		// remove empty values from array
		$subs_users = array_filter($subs_users);
		if (in_array($_SERVER['PHP_AUTH_USER'],$subs_users)) {
			$print_sub=1;
		} else {
			$print_sub=0;
		}
	} else {
		$print_sub=1;
	}

	if ($print_config || $print_sub) {
		$lists .= "<p>".htmlentities($mlmmj_list)."<br/>\n";

		if ($print_config) {
			$lists .= "<a href=\"edit.php?list=".urlencode($mlmmj_list)."\">Config</a> - ";
			$lists .= "<a href=\"texts.php?list=".urlencode($mlmmj_list)."\">Text files</a>";
		}
		if ($print_config && $print_sub)
			$lists .= " - ";
		if ($print_sub)
			$lists .= "<a href=\"subscribers.php?list=".urlencode($mlmmj_list)."\">Subscribers</a>\n";

		if ($print_config)
			$lists .= " - <a href=\"logs.php?list=".urlencode($mlmmj_list)."\">Log</a>";

		$lists .= "</p>\n";
	}
    }
}

$tpl->assign(array("LISTS" => $lists));


$tpl->parse("MAIN","main");
$tpl->FastPrint("MAIN");

?>
