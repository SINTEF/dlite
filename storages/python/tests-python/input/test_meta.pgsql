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
-- Name: http://onto-ns.com/meta/0.3/EntitySchema; Type: TABLE; Schema: public; Owner: dlite_test
--

CREATE TABLE public."http://onto-ns.com/meta/0.3/EntitySchema" (
    uuid character(36) NOT NULL,
    uri character varying,
    meta character varying,
    shape integer[],
    name character varying,
    version character varying,
    namespace character varying,
    description character varying,
    dimensions character varying[],
    properties character varying[]
);


ALTER TABLE public."http://onto-ns.com/meta/0.3/EntitySchema" OWNER TO dlite_test;

--
-- Name: uuidtable; Type: TABLE; Schema: public; Owner: dlite_test
--

CREATE TABLE public.uuidtable (
    uuid character(36) NOT NULL,
    meta character varying
);


ALTER TABLE public.uuidtable OWNER TO dlite_test;

--
-- Data for Name: http://onto-ns.com/meta/0.3/EntitySchema; Type: TABLE DATA; Schema: public; Owner: dlite_test
--

COPY public."http://onto-ns.com/meta/0.3/EntitySchema" (uuid, uri, meta, shape, name, version, namespace, description, dimensions, properties) FROM stdin;
2b10c236-eb00-541a-901c-046c202e52fa	http://onto-ns.com/meta/0.1/test-entity	http://onto-ns.com/meta/0.3/EntitySchema	{3,6}	test-entity	0.1	http://onto-ns.com/meta	test entity with explicit meta	{{L,"first dim"},{M,"second dim"},{N,"third dim"}}	{{myblob,blob3,"","","",""},{mydouble,float64,"",m,"","A double following a single character..."},{myfixstring,string3,"","","","A fix string."},{mystring,string,"","","","A string pointer."},{myshort,uint16,"","","","An unsigned short integer."},{myarray,int32,"L,M,N","","","An array string pointer."}}
\.


--
-- Data for Name: uuidtable; Type: TABLE DATA; Schema: public; Owner: dlite_test
--

COPY public.uuidtable (uuid, meta) FROM stdin;
2b10c236-eb00-541a-901c-046c202e52fa	http://onto-ns.com/meta/0.3/EntitySchema
\.


--
-- Name: http://onto-ns.com/meta/0.3/EntitySchema http://onto-ns.com/meta/0.3/EntitySchema_pkey; Type: CONSTRAINT; Schema: public; Owner: dlite_test
--

ALTER TABLE ONLY public."http://onto-ns.com/meta/0.3/EntitySchema"
    ADD CONSTRAINT "http://onto-ns.com/meta/0.3/EntitySchema_pkey" PRIMARY KEY (uuid);


--
-- Name: uuidtable uuidtable_pkey; Type: CONSTRAINT; Schema: public; Owner: dlite_test
--

ALTER TABLE ONLY public.uuidtable
    ADD CONSTRAINT uuidtable_pkey PRIMARY KEY (uuid);


--
-- PostgreSQL database dump complete
--

