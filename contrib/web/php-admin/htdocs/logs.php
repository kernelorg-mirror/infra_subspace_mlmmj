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

function mask($str, $first, $last) {
    $len = strlen($str);
    $toShow = $first + $last;
    return substr($str, 0, $len <= $toShow ? 0 : $first).str_repeat("*", $len - ($len <= $toShow ? 0 : $toShow)).substr($str, $len - $last, $len <= $toShow ? 0 : $last);
}

function mask_email($email) {
    $mail_parts = explode("@", $email);
    $domain_parts = explode('.', $mail_parts[1]);

    $mail_parts[0] = mask($mail_parts[0], 2, 1); // show first 2 letters and last 1 letter
    $domain_parts[0] = mask($domain_parts[0], 2, 1); // same here
    $mail_parts[1] = implode('.', $domain_parts);

    return implode("@", $mail_parts);
}

function anonymize_line($text) {
    $words = explode(" ", $text);
    foreach ($words as $key => $word) {
	    if (strstr($word, '@')) {
		    $words[$key] = mask_email($word);
	    }
    }
    return implode(" ", $words);
}

function read_filetail($file, $lines) {
    //global $fsize;
    $handle = fopen($file, "r");
    $linecounter = $lines;
    $pos = -2;
    $beginning = false;
    $text = array();
    while ($linecounter > 0) {
        $t = " ";
        while ($t != "\n") {
            if(fseek($handle, $pos, SEEK_END) == -1) {
                $beginning = true; 
                break; 
            }
            $t = fgetc($handle);
            $pos --;
        }
        $linecounter --;
        if ($beginning) {
            rewind($handle);
        }
        $text[$lines-$linecounter-1] = fgets($handle);
        if ($beginning) break;
    }
    fclose ($handle);
    //return array_reverse(array_map('anonymize_line',$text));
    return array_reverse($text);
}

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

$logfile = $topdir."/".$list."/mlmmj.operation.log";
if (is_file($logfile)) {
	$content = read_filetail($logfile,50);
	print "Output (last 50 lines) of operations logfile: <br /><br />\n";
	print nl2br(htmlentities(join('',$content))); 
} else {
	print "Operations logfile doesn't exist (yet)<br />\n";
}

print "<br /><a href='index.php'>Index</a>";
?>
