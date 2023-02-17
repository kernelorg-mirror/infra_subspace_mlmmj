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

if(empty($_GET['list']))
	die("no list specified");
$list = $_GET['list'];

if (dirname(realpath($topdir."/".$list)) != realpath($topdir))
die("list outside topdir");

if(!is_dir($topdir."/".$list))
die("non-existent list");

$userfile = "$topdir/$list/control/admin_users";
if (is_file($userfile)) {
	// read users into array
        $admin_users = array_map('trim',file($userfile));
	// remove empty values from array
	$admin_users = array_filter($admin_users);
        if (!isset($_SERVER['PHP_AUTH_USER']) || !in_array($_SERVER['PHP_AUTH_USER'],$admin_users)) {
                header("WWW-Authenticate: " .
                        "Basic realm=\"Mlmmj Protected Area\"");
                header("HTTP/1.0 401 Unauthorized");
                //Show failure text, which browsers usually
                //show only after several failed attempts
                print("This page is protected by HTTP " .
                        "Authentication.<br>\n");
                exit;
        }
}

$files = glob($topdir."/".$list."/text/*");

if (isset($_POST['save_texts'])) {
	foreach ($files as $mlmmj_file) {
		$name = basename($mlmmj_file);
		// values need to be in linux line endings
		$value = str_replace(array("\r\n", "\r", "\n"), "\n", $_POST[$name]);
		file_put_contents($topdir."/".$list."/text/".$name, $value);
	}
	print "###### <br />\n";
	print "###### Files updated <br />\n";
	print "###### <br />\n";
} 

print "For info on the template directives used, see <a href='http://mlmmj.org/docs/readme-listtexts/'>here</a><br />\n";

print "<form action=\"texts.php?list=".htmlspecialchars($list)."\" method=\"post\" style=\"margin: 0; margin-left: 1em\">";

print "<table style='width: 95%; margin: 1em;'>";
foreach ($files as $mlmmj_file) {
	$name=basename($mlmmj_file);
	$content = file_get_contents($mlmmj_file);
	print "<tr><td style='width: 10%;'>".htmlentities($name)."</td><td><textarea name='$name' rows='10' style='width: 95%;'>". htmlentities($content). "</textarea></td></tr>";
}
print '</table>';
print "<input type=\"submit\" name=\"save_texts\" value=\"Save\" />";
print "</form>";
print "<br /><a href='index.php'>Index</a>";
?>
