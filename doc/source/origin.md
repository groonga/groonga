# The Pre-Groonga Era

By Naozumi Takenaka (Founder and current CEO of Mirai Kensaku -
meaning Future Search - Brazil)

December 2024

## The Internet Landscape at that time

Around 2003, as Google's dominance in internet search became
established, many major Japanese services including Yahoo! Japan began
adopting Google's search engine. The development of Groonga originated
when NTT's goo search service made the business decision to abandon
their proprietary search engine and crawler system.

At the time, it was clear not only to engineers but also to many users
who viewed the internet as a cultural phenomenon that search engines
were an indispensable technology for finding relevant information
among the explosively growing content post-internet. Search engines
had the potential to not just control but fundamentally change user
behavior online.

From a software engineering perspective, search engines are a complex
bundle of critical technologies including tokenization, tree
construction, indexing methods, performance optimization for each
component, and multilingual support - not something that can be
achieved with half-hearted efforts. The same applies to crawler
technology. Japanese language processing in particular presents
numerous challenges absent in Western languages, such as tokenization,
verb conjugation variations, mixed character sets, and kanji
variants. Search engine technology for Japanese builds upon detailed
technical expertise accumulated since the personal computer era. As an
engineer working in the Japanese language sphere, I felt an urgent
sense of crisis seeing this technological accumulation being casually
discarded in favor of "foreign-made" engines. The "Google is good
enough" decision in 2003 exemplified how software was being treated as
merely a commodity in Japan's internet business landscape, with little
regard for its cultural aspects and value.

## The Birth of Brazil Inc. and Senna

To counter this situation, Mirai Kensaku - meaning Future Search -
Brazil LLC (hereinafter "Brazil") was founded. Due to legal
requirements at the time mandating minimum capital of 10 million yen
for stock companies, we opted for an LLC structure with 3 million yen
in capital. One of the founders, Hiroyuki Nishimura, was then the
administrator and developer of 2channel (2ch), which lacked
comprehensive cross-board search functionality - there were only a few
volunteer-run services offering limited search capabilities. This
presented an opportunity to repurpose NTT's soon-to-be-abandoned
search engine technology for 2ch.

For about two years after establishing Brazil, we built the 2ch search
service using NTT's licensed search engine software (eva) while
simultaneously conceptualizing and developing an entirely new,
original search engine. During this period, we also established our
commitment to open source, which continues to this day. This new
engine was named Senna - combining Brazil's "Brazil" connection with
an emphasis on speed. While many pronounced it "sen-na", the correct
pronunciation is "se-na" given this background.

Around 2005, Livedoor was at its peak in Japan, with CEO Takafumi
Horie publicly expressing his desire to build a search service that
could compete with Google. Brazil saw this as a once-in-a-lifetime
opportunity and planned to extend Senna with distributed server
support and other features to create a large-scale search service that
could match Google's speed and accuracy at the time. While these plans
were ultimately derailed by the Livedoor incident in 2006, their
implementation might have altered the course of the internet history.

## The Evolution to Groonga

Shortly after Senna went into operation, one of Brazil's founders,
Daijiro Mori, exemplified the programming adage that "you should write
the same software three times" by initiating the development of
Groonga as the third iteration. Throughout this period, Brazil
maintained its clear vision of preserving pure Japanese search engine
technology for the future.

Regarding Groonga's name, Mori provided the true story:

> I named it Groonga after being strongly impressed by something
> Hideya Adachi of Fukuoka's progressive band 'Takenouchi Quartet'
> told me - that tracing the origins of the blue note scale leads to
> Grunga village in East Africa.

This naming approach differed completely from Senna's - perhaps we
could have renamed the company to "Future Search Africa" at that point
:-)

Subsequently, we decided to not just make it open source but to
entrust development to the community, believing this would enable
multifaceted, evolutionary development unrestricted by Brazil's
internal resources - including debugging, feature expansion, and
documentation enhancement. This led to strengthening links with
various developers, including ClearCode Inc.

I'll end this section hoping that contributors will continue writing
this history.
