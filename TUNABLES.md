
TUNABLES for lists managed with mlmmj
=====================================

The following files can be used for changing the behaviour of a list. The
filename is supposed to be below listdir/control. In the case it's a "boolean",
the contents of a file does not matter, the mere presence of it, will set the
variable to "true". If it's a "normal" file, the first line will be used as
value, leaving line 2 and forward ready for commentary etc. If it's possible
to specify several entries (one pr. line), it's marked "list". If the file's
entire content is used as value, it's marked "text".

 * autosubscribe        (boolean)

   In case of a moderated mailing list, if the incoming email is accepted by
   the moderator, then the sender it automatically subscribed as `nomail`.

 * listaddress			(list)

   This file contains all addresses which mlmmj sees as listaddresses (see
   tocc below). The first one is the one used as the primary one, when mlmmj
   sends out mail.

 * closedlist			(boolean)

   Is the list is open or closed. If it's closed subscription and
   unsubscription via mail is disabled.

 * closedlistsub		(boolean)

   Closed for subscription. Unsubscription is possible.

 * moderated			(boolean)

   If this file is present, the emailaddresses in the file
   listdir/control/moderators will act as moderators for the list.

 * submod			(list)

   If this file is present, subscription will be moderated by owner(s).
   If there are emailaddresses in this file, then these will be used
   instead of owner.

 * tocc				(boolean)

   If this file is present, the list address does not have to be in the `To:`
   or `Cc:` header of the email to the list.

 * subonlypost			(boolean)

   When this file is present, only people who are subscribed to the list,
   are allowed to post to it. The check is made against the `From:` header.
   If a non-subscriber posts and the list has no moderators
   (`control/moderators` does not exist), the sender receives a denial mail
   with an option to simultaneously subscribe and release their post.
   When moderators are configured, this self-release option is disabled and
   the post is simply denied.
   This tunable takes precedence over `modonlypost`: if both are present,
   only the subscriber check is performed.

 * subreleaseall		(boolean)

   When this file is present, a sender using the self-release option of
   `subonlypost` (or `modnonsubposts`) also releases every other message
   of theirs still awaiting moderation, instead of only the one they
   replied to. This is meant for batches of mail such as a patch series
   sent before subscribing. Messages are matched against the `From:`
   header of the held mail.

 * modonlypost			(boolean)

   When this file is present, only people listed in listdir/control/moderators
   are allowed to post to it. The check is made against the `From:` header.
   Ignored if `subonlypost` is also present.

 * modnonsubposts		(boolean)

   When this file is present, all postings from people who are not allowed
   to post to the list will be moderated instead of denied.
   When used without `modonlypost`, it implies `subonlypost`.

 * modreqlife			(normal)

   This specifies how long in seconds a mail awaits moderation before it's
   discarded. Defaults to 604800 seconds, which is 7 days.

 * prefix			(normal)

   The prefix for the `Subject:` line of mails to the list. This will alter the
   `Subject:` line, and add a prefix if it's not present elsewhere.

 * replyto			(boolean) DEPRECATED

   When this file is present, the `From:` line of mails will be added as a
   `Reply-To:` header. This allows the mail to be delivered safely when DMARC
   protected emails are received from the list.

   This control is deprecated: add a `Reply-To:` header to `customheaders`
   instead (see the DKIM recipe in `README.md`). Using both results in two
   `Reply-To:` headers, which is not RFC 5322 compliant and gets the mail
   rejected by several large providers.

 * owner			(list)

   The emailaddresses in this file (1 pr. line) will get mails to
   listname+owner@listdomain.tld

 * customheaders		(list)

   These headers are added to every mail coming through. This is the place you
   want to add `Reply-To:` header in case you want such.

 * delheaders			(list)

   This file specifies one header token to match per line. If the file contains
   entries such as `Received:` and `Message-ID:`, then all occurrences of these
   headers in incoming mail will be deleted.

   Additionally, the headers `From ` and `Return-Path:` will be deleted regardless
   of their presence in the list.

   **Important Note on Header Matching:**

   The header matching is based on prefixes. This means that:

   - A header token specified as `X-:` will match only headers that are exactly
     `X-`.
   - A header token specified as `X-` will match all headers that begin with the
     prefix `X-`, including headers like `X-Example`, `X-Header-Name`, etc.

   Make sure to specify the header tokens according to these rules to ensure
   the desired headers are correctly matched and deleted.

 * access			(list)

   If this file exists, all headers of a post to the list is matched against
   the rules. The first rule to match wins. See README.access for syntax and
   examples.

 * addtohdr			(boolean)

   When this file is present, a `To:` header including the recipients
   emailaddress will be added to outgoing mail. Recommended usage is to remove
   existing `To:` headers with delheaders (see above) first.

 * relayhost			(normal)

   The host specified (IP address or hostname, both works) in this file will
   be used for relaying the mail sent to the list. Defaults to 127.0.0.1.

 * notifysub			(boolean)

   If this file is present, the owner(s) will get a mail with the address of
   someone sub/unsubscribing to a mailinglist.

 * notifymod			(boolean)

   If this file is present, the poster (based on the envelope from) will
   get a mail when their post is being moderated.

 * digestinterval		(normal)

   This file specifies how many seconds will pass before the next digest is
   sent. Defaults to 604800 seconds, which is 7 days.

 * digestmaxmails		(normal)

   This file specifies how many mails can accumulate before digest sending is
   triggered. Defaults to 50 mails, meaning that if 50 mails arrive to the list
   before digestinterval have passed, the digest is delivered.

 * bouncelife			(normal)

   This specifies how long in seconds an address can bounce before it's
   unsubscribed. Defaults to 432000 seconds, which is 5 days.

 * noarchive			(boolean)

   If this file exists, the mail won't be saved in the archive but simply
   deleted.

 * archivepartition		(boolean)

   If this file exists, archived mails are stored in a YYYY/MM/ directory
   hierarchy under `archive/` (e.g. `archive/2026/03/42`) instead of the
   default flat layout (`archive/42`). Existing flat archives are
   automatically migrated by `mlmmj-maintd` based on file modification
   time. Reading always works regardless of layout.

 * nosubconfirm			(boolean)

   If this file exists, no mail confirmation is needed to subscribe to the
   list. This should in principle never ever be used, but there are times
   on local lists etc. where this is useful. HANDLE WITH CARE!

 * noget			(boolean)

   If this file exists, then retrieving old posts with +get-N is disabled.

 * subonlyget			(boolean)

   If this file exists, then retrieving old posts with +get-N is only
   possible for subscribers. The above mentioned 'noget' have precedence.

 * verp				(normal)

   Control how Mlmmj does VERP (variable envelope return path). If this tunable
   does not exist, Mlmmj will send a message to the SMTP server for each
   recipient, with an appropriate envelope return path, i.e. it will handle
   VERP itself. If the tunable does exist, Mlmmj will instead divide the
   recipients into groups (the maximum number of recipients in a group can be
   controlled by the maxverprecips tunable) and send one message to the SMTP
   server per group. The content of this tunable allows VERP to be handled by
   the SMTP server. If the tunable contains "postfix", Mlmmj will make Postfix
   use VERP by adding XVERP=-= to the `MAIL FROM:` line. If it contains something
   else, that text will be appended to the `MAIL FROM:` line. If it contains
   nothing, VERP will effectively be disabled, as neither Mlmmj nor the SMTP
   server will do it.

 * maxverprecips		(normal)

   How many recipients per mail delivered to the SMTP server. Defaults to 100.

 * notoccdenymails		(boolean)
 * noaccessdenymails		(boolean)
 * nosubonlydenymails		(boolean)
 * nomodonlydenymails		(boolean)

   These switches turns off whether mlmmj sends out notification about postings
   being denied due to the listaddress not being in `To:` or `Cc:` (see 'tocc'),
   when it was rejected due to an access rule (see 'access') or whether it's a
   subscribers/moderators only posting list (see 'subonlypost/modonlypost').

 * nosubmodmails		(boolean)

   This switch turns off whether mlmmj sends out notification about
   subscription being moderated to the person requesting subscription
   (see 'submod').

 * smtpport			(normal)

   In this file a port other than port 25 for connecting to the relayhost
   can be specified.

 * delimiter			(normal)

   This specifies what to use as recipient delimiter for the list.
   Default is "+".

 * nodigesttext			(boolean)

   If this file exists, digest mails won't have a text part with a thread
   summary.

 * nodigestsub			(boolean)

   If this file exists, subscription to the digest version of the mailinglist
   will be denied. (Useful if you don't want to allow digests and notify users
   about it).

 * nonomailsub			(boolean)

   If this file exists, subscription to the nomail version of the mailinglist
   will be denied. (Useful if you don't want to allow nomail and notify users
   about it).

 * maxmailsize			(normal)

   With this option the maximal allowed size of incoming mails can be specified.

 * mimedeny			(list)

   If this file exists, incoming mails are scanned for their MIME content
   type (`Content-Type:` header). If the type matches one of the entries in
   this file (matched as a prefix), the mail is not rejected but flagged by
   adding the header `X-ThisMailContainsUnwantedMimeParts: Y`. For
   multipart messages, the content types of nested parts are checked too.
   Example: `text/html`.

 * nomaxmailsizedenymails	(boolean)

   If this is set, no reject notifications caused by violation of maxmailsize
   will be sent.

 * nolistsubsemail		(boolean)

   If this is set, the LISTNAME+list@ functionality for requesting an
   email with the subscribers for owner is disabled.

 * staticbounceaddr		(normal)

   If this is set to something@example.org, the bounce address (`Return-Path:`)
   will be fixed to something+listname-bounces-and-so-on@example.org
   in case you need to disable automatic bounce handling.

 * ifmodsendonlymodmoderate	(boolean)

   If this file is present, then mlmmj in case of moderation checks the
   envelope from, to see if the sender is a moderator, and in that case
   only send the moderation mails to that address. In practice this means that
   a moderator sending mail to the list won't bother all the other moderators
   with his mail.

 * footer			(text)

   The content of this file is appended to mail sent to the list.

 * notmetoo			(boolean)

   If this file is present, mlmmj attempts to exclude the sender of a post
   from the distribution list for that post so people don't receive copies
   of their own posts.

 * smtphelo			(normal)

   When this file is present, it contains the hostname to send in the SMTP
   EHLO or HELO command. Otherwise the machine hostname is used.

 * listidphrase		(normal)

   The optional phrase (display name) inserted before the `<list-id>` in the
   `List-Id:` header, as allowed by RFC 2919: `List-Id: [phrase] <list-id>`.
   The value is used verbatim, so it has to be a valid RFC 5322 phrase; encode
   non-ASCII text as an RFC 2047 encoded word. When the file is absent or
   empty, the header is `List-Id: <name.fqdn>`. Ignored when `nolistheaders`
   is set.

 * nolistheaders		(boolean)

   If this file exists, mlmmj will not add the automatic RFC 2919/2369
   `List-Id`, `List-Post`, `List-Help`, `List-Subscribe`, `List-Unsubscribe`,
   `List-Owner` (when `control/owner` exists), and `Precedence: list` headers.
   These headers are generated by default, on posts and on digests, to
   improve deliverability and allow mail clients to detect list messages.

 * xforwardedto		(boolean)

   If this file exists, an `X-Forwarded-To:` header containing the
   recipient's address is added to each outgoing message. This helps
   providers like Gmail recognize legitimate mailing list forwarding.
   Incompatible with VERP (per-recipient header injection is required).

 * dara			(boolean)

   If this file exists, an `X-Signed-Recipient:` header is added to each
   outgoing message for DARA (ARC replay-resistant authentication) support.
   The header proves the message was intended for a specific recipient.
   Incompatible with VERP (per-recipient header injection is required).

 * send			(list)

   The file contains a list of email address for which the mail will be sent
   unconditionally. It will not be moderated, nor subject to subonlypost, nor
   modnonsubposts.
