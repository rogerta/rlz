An RLZ URL parameter is a string consisting of many parts:

![http://rlz.googlecode.com/files/rlz_exploded.png](http://rlz.googlecode.com/files/rlz_exploded.png)

  * **rlz encoding** is at version 1 as of Sept 2007, but this will be incremented each time the encoding scheme changes.
  * **application** T=Toolbar, B=Firefox Toolbar, D=Desktop (authoritative list in the [source code](https://code.google.com/p/rlz/source/browse/trunk/lib/lib_values.cc)) With one digit for major version.
  * **brand code** identifies distribution channel, may be a partner or internal marketing. This correlates to how the user got the software (ie. they downloaded it by itself vs. it came pre-installed on their new computer vs. it came with a partner's software).
  * **cannibal** tells if the library has evidence that the user was a user prior to installing the software. It's from the term "cannibalization" which refers to the fact that users who get client products through distribution were already loyal customers, thus impacting the actual incremental value of the channel.
  * **language** is the two- (en) or five-character (zh-CN) language code of the application. Valid values depend on the specific app.
  * **install cohort** is the country and week of the user's installation event. Country is determined by the server, using IP address. Week is measured as number of weeks since Feb 3, 2003. It's used to measure attrition rate and is used in ROI and accounting analyses.
  * **first search cohort** is just like install cohort, but for first search event. They're both called "life cycle events".


Other specifics of RLZ parameter strings:
  * **rlz encoding**, **application**, **brand code**, **language**, and **install cohort** are the required fields, and are the minimum that will appear in an RLZ string.
    * Underscores generally indicate a missing value.  An underscore where the **cannibal** signal should be indicates that this is not a cannibalization.
    * **first search cohort** may not appear.  If only one 'cohort' value appears in a string, it must be the **install cohort**
  * The **brand code** can be any four alphabetic characters, all upper-case.  There is not exhaustive enumeration, as this can be any four-alphabetic-character value you want.
  * The only **rlz encoding** value at this time is "1".

For example, if the string reads:

```
rlz=1T4AAAA_enUS236
```

This means this installation was not a cannibalization and is not (yet) part of a first-search cohort.

In a very small number of cases, if network or registry key interactions fail, the install cohort will be replaced by underscores, as so:

```
rlz=1T4AAAA_en_____US239
```
These instances are rare, however.