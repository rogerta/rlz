**NOTE: As of 6/21/2012, the source code has moved to the Chromium repository. The instructions in the wiki have been updated.**

The RLZ project is a library for grouping promotion event signals and anonymous user cohorts.

![http://rlz.googlecode.com/files/rlz_exploded.png](http://rlz.googlecode.com/files/rlz_exploded.png)

Client applications with the RLZ library can use explicit cohort tagging to manage promotion analysis.  A client application with a particular tag can transmit that tag as it chooses for payments and analysis purposes.  As an example, the RLZ parameter "rlz=1T4AAAA\_enUS202" indicates the client application is Toolbar version 4, distributed with AAAA software bundle, English version, to a US user in December 2006. This empowers computation of metrics broken down into useful dimensions.

Find out more about [how to read RLZ strings](https://github.com/rogerta/rlz/blob/wiki/HowToReadAnRlzString.md), how the code [works](https://github.com/rogerta/rlz/tree/master/lib) in general, and how it is used in the [Google Chrome](http://www.google.com/intl/en/landing/chrome/google-chrome-privacy-whitepaper.pdf) web browser.
