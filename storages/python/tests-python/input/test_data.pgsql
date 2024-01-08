--
-- PostgreSQL database dump
--

-- Dumped from database version 14.1
-- Dumped by pg_dump version 14.1

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: http://onto-ns.com/meta/0.1/test-entity; Type: TABLE; Schema: public; Owner: dlite_test
--

CREATE TABLE public."http://onto-ns.com/meta/0.1/test-entity" (
    uuid character(36) NOT NULL,
    uri character varying,
    meta character varying,
    shape integer[],
    myblob bytea,
    mydouble double precision,
    myfixstring character varying,
    mystring character varying,
    myshort integer,
    myarray integer[]
);


ALTER TABLE public."http://onto-ns.com/meta/0.1/test-entity" OWNER TO dlite_test;

--
-- Name: uuidtable; Type: TABLE; Schema: public; Owner: dlite_test
--

CREATE TABLE public.uuidtable (
    uuid character(36) NOT NULL,
    meta character varying
);


ALTER TABLE public.uuidtable OWNER TO dlite_test;

--
-- Data for Name: http://onto-ns.com/meta/0.1/test-entity; Type: TABLE DATA; Schema: public; Owner: dlite_test
--

COPY public."http://onto-ns.com/meta/0.1/test-entity" (uuid, uri, meta, shape, myblob, mydouble, myfixstring, mystring, myshort, myarray) FROM stdin;
204b05b2-4c89-43f4-93db-fd1cb70f54ef	\N	http://onto-ns.com/meta/0.1/test-entity	{2,2,3}	\\x0a0120	2.72	Si	...	13	{{{1,2,3},{3,2,1}},{{4,5,6},{6,5,4}}}
e076a856-e36e-5335-967e-2f2fd153c17d	my_test_instance	http://onto-ns.com/meta/0.1/test-entity	{2,1,3}	\\x0a0010	3.14	Al	allocated string...	17	{{{1,2,3}},{{4,5,6}}}
\.


--
-- Data for Name: uuidtable; Type: TABLE DATA; Schema: public; Owner: dlite_test
--

COPY public.uuidtable (uuid, meta) FROM stdin;
204b05b2-4c89-43f4-93db-fd1cb70f54ef	http://onto-ns.com/meta/0.1/test-entity
e076a856-e36e-5335-967e-2f2fd153c17d	http://onto-ns.com/meta/0.1/test-entity
\.


--
-- Name: http://onto-ns.com/meta/0.1/test-entity http://onto-ns.com/meta/0.1/test-entity_pkey; Type: CONSTRAINT; Schema: public; Owner: dlite_test
--

ALTER TABLE ONLY public."http://onto-ns.com/meta/0.1/test-entity"
    ADD CONSTRAINT "http://onto-ns.com/meta/0.1/test-entity_pkey" PRIMARY KEY (uuid);


--
-- Name: uuidtable uuidtable_pkey; Type: CONSTRAINT; Schema: public; Owner: dlite_test
--

ALTER TABLE ONLY public.uuidtable
    ADD CONSTRAINT uuidtable_pkey PRIMARY KEY (uuid);


--
-- PostgreSQL database dump complete
--

